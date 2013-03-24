/* Name: main.c
 * Project: Micronucleus
 * Author: Jenna Fox
 * Creation Date: 2007-12-08
 * Tabsize: 4
 * Copyright: (c) 2012 Jenna Fox
 * Portions Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH (USBaspLoader)
 * Portions Copyright: (c) 2012-2013 Louis Beaudoin (USBaspLoader-tiny85 and new contributions)
 * License: GNU GPL v2 (see License.txt)
 */
 
#define MICRONUCLEUS_VERSION_MAJOR 2
#define MICRONUCLEUS_VERSION_MINOR 0
// how many milliseconds should host wait till it sends another erase or write?
// needs to be above 4.5 (and a whole integer) as avr freezes for 4.5ms
// in current testing PC application fails if delay is less than 8ms, not sure why
#define MICRONUCLEUS_WRITE_SLEEP 8


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
//#include <avr/eeprom.h>
#include <util/delay.h>
//#include <string.h>

static void leaveBootloader() __attribute__((__noreturn__));

#include "bootloaderconfig.h"
#include "usbdrv/usbdrv.c"

/* ------------------------------------------------------------------------ */

#ifndef ulong
#   define ulong    unsigned long
#endif
#ifndef uint
#   define uint     unsigned int
#endif

#ifndef BOOTLOADER_CAN_EXIT
#   define  BOOTLOADER_CAN_EXIT     0
#endif

/* allow compatibility with avrusbboot's bootloaderconfig.h: */
#ifdef BOOTLOADER_INIT
#   define bootLoaderInit()         BOOTLOADER_INIT
#   define bootLoaderExit()
#endif
#ifdef BOOTLOADER_CONDITION
#   define bootLoaderCondition()    BOOTLOADER_CONDITION
#endif

/* device compatibility: */
#ifndef GICR    /* ATMega*8 don't have GICR, use MCUCR instead */
#   define GICR     MCUCR
#endif

/* ------------------------------------------------------------------------ */

//#define addr_t uint
// typedef union longConverter{
//     addr_t  l;
//     uint    w[sizeof(addr_t)/2];
//     uchar   b[sizeof(addr_t)];
// } longConverter_t;

//////// Stuff Bluebie Added
// postscript are the few bytes at the end of programmable memory which store tinyVectors
// and used to in USBaspLoader-tiny85 store the checksum iirc
#define POSTSCRIPT_SIZE 4
#define PROGMEM_SIZE (BOOTLOADER_ADDRESS - POSTSCRIPT_SIZE) /* max size of user program */

// verify the bootloader address aligns with page size
#if BOOTLOADER_ADDRESS % SPM_PAGESIZE != 0
#  error "BOOTLOADER_ADDRESS in makefile must be a multiple of chip's pagesize"
#endif

#ifdef AUTO_EXIT_MS
#  if AUTO_EXIT_MS < (MICRONUCLEUS_WRITE_SLEEP * (BOOTLOADER_ADDRESS / SPM_PAGESIZE))
#    warning "AUTO_EXIT_MS is shorter than the time it takes to perform erase function - might affect reliability?"
#    warning "Try increasing AUTO_EXIT_MS if you have stability problems"
#  endif
#endif

// events system schedules functions to run in the main loop
static uchar events = 0; // bitmap of events to run
#define EVENT_EXECUTE 1
#define EVENT_SPM_OPERATION 2

// controls state of events
#define fireEvent(event) events |= (event)
#define isEvent(event)   (events & (event))
#define clearEvents()    events = 0

uint16_t idlePolls = 0; // how long have we been idle?


/* ------------------------------------------------------------------------ */
static uchar usbFunctionSetup(uchar data[8]);
static inline void initForUsbConnectivity(void);
static inline void tiny85FlashInit(void);
static inline void leaveBootloader(void);

// clear memory which stores data to be written by next writeFlashPage call
#define __boot_page_fill_clear()   \
(__extension__({                                 \
    __asm__ __volatile__                         \
    (                                            \
        "sts %0, %1\n\t"                         \
        "spm\n\t"                                \
        :                                        \
        : "i" (_SFR_MEM_ADDR(__SPM_REG)),        \
          "r" ((uint8_t)(__BOOT_PAGE_FILL | (1 << CTPB)))     \
    );                                           \
}))

/* ------------------------------------------------------------------------ */
static uchar fillAddress;
static uint16_t spmAddress;
static uchar spmCommand;

static uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;
    idlePolls = 0; // reset idle polls when we get usb traffic
    
    static uchar replyBuffer[5] = {
        (((uint)PROGMEM_SIZE) >> 8) & 0xff,
        ((uint)PROGMEM_SIZE) & 0xff,
        SPM_PAGESIZE,
        MICRONUCLEUS_WRITE_SLEEP,
        POSTSCRIPT_SIZE
    };
    
    if (rq->bRequest == 0) { // get device info
        usbMsgPtr = (usbMsgPtr_t)replyBuffer;
        return 5;
        
    } else if (rq->bRequest & SPM_COMMAND_MASK) { // spm operation
        spmCommand = rq->bRequest & ~(SPM_COMMAND_MASK);
        spmAddress = rq->wIndex.word;
        fireEvent(EVENT_SPM_OPERATION);       

    } else if (rq->bRequest & LOADPAGE_COMMAND_MASK) {
        cli();
        boot_page_fill(rq->bRequest & ~LOADPAGE_COMMAND_MASK, rq->wIndex.word);
        sei();

    } else { // exit bootloader
#       if BOOTLOADER_CAN_EXIT
            fireEvent(EVENT_EXECUTE);
#       endif
    }
    
    return 0;
}

/* ------------------------------------------------------------------------ */

// this tells the linker to call this function right after the stack is initialized, before main()
void PushMagicWord (void) __attribute__ ((naked)) __attribute__ ((section (".init3")));

// put the word "B007" at the bottom of the stack (RAMEND - RAMEND-1)
void PushMagicWord (void) {
    asm volatile("ldi r16, 0xB0"::);
    asm volatile("push r16"::);
    asm volatile("ldi r16, 0x07"::);
    asm volatile("push r16"::);
}

/* ------------------------------------------------------------------------ */

static inline void initForUsbConnectivity(void) {
    usbInit();
    /* enforce USB re-enumerate: */
    usbDeviceDisconnect();  /* do this while interrupts are disabled */
    _delay_ms(500);
    usbDeviceConnect();
    sei();
}

// this function should be called with interrupts disabled as the boot_page instructions can have
// no break between loading SPMCSR and executing SPM instruction
static inline void tiny85FlashInit(void) {
    // clean up the boot page registers, in case there was a partial page already loaded
    // this is a good place to do it as it's run during initialization, and after writing/erasing flash
    __boot_page_fill_clear();

    // check for erased first page (no bootloader interrupt vectors), add vectors if missing
    // this needs to happen for usb communication to work later - essential to first run after bootloader
    // being installed
    if(pgm_read_word(RESET_VECTOR_OFFSET * 2) != 0xC000 + (BOOTLOADER_ADDRESS/2) - 1 ||
            pgm_read_word(USBPLUS_VECTOR_OFFSET * 2) != 0xC000 + (BOOTLOADER_ADDRESS/2) - 1) {
        boot_page_fill(RESET_VECTOR_OFFSET * 2, 0xC000 + (BOOTLOADER_ADDRESS/2) - 1);
        boot_page_fill(USBPLUS_VECTOR_OFFSET * 2, 0xC000 + (BOOTLOADER_ADDRESS/2) - 1);
        boot_page_write(0);
        boot_spm_busy_wait(); // Wait until the memory is written.
    }
}

// reset system to a normal state and launch user program
static inline void leaveBootloader(void) {
    _delay_ms(10); // removing delay causes USB errors
    
    //DBG1(0x01, 0, 0);
    bootLoaderExit();
    cli();
    USB_INTR_ENABLE = 0;
    USB_INTR_CFG = 0;       /* also reset config bits */

    // clear magic word from bottom of stack before jumping to the app
    *(uint8_t*)(RAMEND) = 0x00;
    *(uint8_t*)(RAMEND-1) = 0x00;
    
    // adjust clock to previous calibration value, so user program always starts with same calibration
    // as when it was uploaded originally
    // TODO: Test this and find out, do we need the +1 offset?
    unsigned char stored_osc_calibration = pgm_read_byte(BOOTLOADER_ADDRESS - TINYVECTOR_OSCCAL_OFFSET);
    if (stored_osc_calibration != 0xFF && stored_osc_calibration != 0x00) {
        //OSCCAL = stored_osc_calibration; // this should really be a gradual change, but maybe it's alright anyway?
        // do the gradual change - failed to score extra free bytes anyway in 1.06
        while (OSCCAL > stored_osc_calibration) OSCCAL--;
        while (OSCCAL < stored_osc_calibration) OSCCAL++;
    }

    // jump to application reset vector at end of flash
    asm volatile ("rjmp __vectors - 4");
}


#define run_spm_operation(address, instruction)  \
(__extension__({                                 \
    __asm__ __volatile__                         \
    (                                            \
        "sts %0, %1\n\t"                         \
        "spm\n\t"                                \
        :                                        \
        : "i" (_SFR_MEM_ADDR(__SPM_REG)),        \
          "r" ((uint8_t)(instruction)),          \
          "z" ((uint16_t)(address))              \
    );                                           \
}))


static inline void runSpmOperation(void) {
    // make sure an interrupt can't separate loading the SPMCSR register and executing the SPM instruction
    cli();
    run_spm_operation(spmAddress, spmCommand);
    boot_spm_busy_wait(); // Wait until the memory is written.
    
    // load the interrupt vectors in case they were just erased
    tiny85FlashInit();
    sei();
}


int main(void) {
    /* initialize  */
    #ifdef RESTORE_OSCCAL
        uint8_t osccal_default = OSCCAL;
    #endif
    #if (!SET_CLOCK_PRESCALER) && LOW_POWER_MODE
        uint8_t prescaler_default = CLKPR;
    #endif
    
    wdt_disable();      /* main app may have enabled watchdog */
    tiny85FlashInit();
    bootLoaderInit();
    
    if (bootLoaderStartCondition()) {
        #if LOW_POWER_MODE
            // turn off clock prescalling - chip must run at full speed for usb
            // if you might run chip at lower voltages, detect that in bootLoaderStartCondition
            CLKPR = 1 << CLKPCE;
            CLKPR = 0;
        #endif
        
        initForUsbConnectivity();
        do {
            usbPoll();
            
            // delay to allow any current USB traffic to complete before disabling interrupts, and to space out calls to usbPoll
            _delay_us(IDLE_POLL_DELAY_US);
            idlePolls++;

            // runSpmOperation will freeze the chip for ~ 4.5ms, breaking usb protocol, so host
            // needs to wait > 4.5ms before next usb request
            if (isEvent(EVENT_SPM_OPERATION)) runSpmOperation();
#           if BOOTLOADER_CAN_EXIT            
                if (isEvent(EVENT_EXECUTE)) { // when host requests device run uploaded program
                    break;
                }
#           endif
            
            clearEvents();
            
        } while(bootLoaderCondition());  /* main event loop runs so long as bootLoaderCondition remains truthy */
    }
    
    // set clock prescaler to desired clock speed (changing from clkdiv8, or no division, depending on fuses)
    #if LOW_POWER_MODE
        #ifdef SET_CLOCK_PRESCALER
            CLKPR = 1 << CLKPCE;
            CLKPR = SET_CLOCK_PRESCALER;
        #else
            CLKPR = 1 << CLKPCE;
            CLKPR = prescaler_default;
        #endif
    #endif
    
    // slowly bring down OSCCAL to it's original value before launching in to user program
    #ifdef RESTORE_OSCCAL
        while (OSCCAL > osccal_default) { OSCCAL -= 1; }
    #endif
    leaveBootloader();
}

/* ------------------------------------------------------------------------ */
