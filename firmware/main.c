/* Name: main.c
 * Project: USBaspLoader
 * Author: Christian Starkjohann
 * Creation Date: 2007-12-08
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * Portions Copyright: (c) 2012 Louis Beaudoin
 * License: GNU GPL v2 (see License.txt)
 * This Revision: $Id: main.c 786 2010-05-30 20:41:40Z cs $
 */

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

#define UBOOT_VERSION 1
// how many milliseconds should host wait till it sends another write?
// this needs to be above 9, but 20 is only sensible for testing
#define UBOOT_WRITE_SLEEP 12

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

#define addr_t uint

typedef union longConverter{
    addr_t  l;
    uint    w[sizeof(addr_t)/2];
    uchar   b[sizeof(addr_t)];
} longConverter_t;

//////// Stuff Bluebie Added
#define PROGMEM_SIZE (BOOTLOADER_ADDRESS - 6)

// outstanding events for the mainloop to deal with
static uchar events = 0; // bitmap of events to run
#define EVENT_PAGE_NEEDS_ERASE 1
#define EVENT_WRITE_PAGE 2
#define EVENT_EXIT_BOOTLOADER 4

#define fireEvent(event) events |= (event)
#define isEvent(event)   (events & (event))
#define clearEvents()    events = 0

static uchar writeLength;

// becomes 1 when some programming happened
// lets leaveBootloader know if needs to finish up the programming
static uchar didWriteSomething = 0;






static uint16_t         vectorTemp[2];
static addr_t  currentAddress; /* in bytes */


/* ------------------------------------------------------------------------ */

static inline void eraseFlashPage(void) {
    cli();
    boot_page_erase(currentAddress - 2);
    boot_spm_busy_wait();
    sei();
}

static void writeFlashPage(void) {
    didWriteSomething = 1;
    cli();
    boot_page_write(currentAddress - 2);
    boot_spm_busy_wait(); // Wait until the memory is written.
    sei();
}

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

static void writeWordToPageBuffer(uint16_t data) {
    // first two interrupt vectors get replaced with a jump to the bootloader vector table
    if (currentAddress == (RESET_VECTOR_OFFSET * 2) || currentAddress == (USBPLUS_VECTOR_OFFSET * 2)) {
        data = 0xC000 + (BOOTLOADER_ADDRESS/2) - 1;
    }

    if (currentAddress == BOOTLOADER_ADDRESS - TINYVECTOR_RESET_OFFSET) {
        data = vectorTemp[0] + ((FLASHEND + 1) - BOOTLOADER_ADDRESS)/2 + 2 + RESET_VECTOR_OFFSET;
    }
    
    if (currentAddress == BOOTLOADER_ADDRESS - TINYVECTOR_USBPLUS_OFFSET) {
        data = vectorTemp[1] + ((FLASHEND + 1) - BOOTLOADER_ADDRESS)/2 + 1 + USBPLUS_VECTOR_OFFSET;
    }
    
    
    // clear page buffer as a precaution before filling the buffer on the first page
    // TODO: maybe clear on the first byte of every page?
    if (currentAddress == 0x0000) __boot_page_fill_clear();
    
    cli();
    boot_page_fill(currentAddress, data);
    sei();
    
	// only need to erase if there is data already in the page that doesn't match what we're programming
	// TODO: what about this: if (pgm_read_word(currentAddress) & data != data) { ??? should work right?
	//if (pgm_read_word(currentAddress) != data && pgm_read_word(currentAddress) != 0xFFFF) {
    if ((pgm_read_word(currentAddress) & data) != data) {
        fireEvent(EVENT_PAGE_NEEDS_ERASE);
    }
    
    currentAddress += 2;
}

static void fillFlashWithVectors(void) {
    int16_t i;

    // fill all or remainder of page with 0xFFFF
    for (i = currentAddress % SPM_PAGESIZE; i < SPM_PAGESIZE; i += 2) {
        writeWordToPageBuffer(0xFFFF);
    }

    writeFlashPage();
}

/* ------------------------------------------------------------------------ */

static uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;
    static uchar replyBuffer[5] = {
        UBOOT_VERSION,
        (((uint)PROGMEM_SIZE) >> 8) & 0xff,
        ((uint)PROGMEM_SIZE) & 0xff,
        SPM_PAGESIZE,
        UBOOT_WRITE_SLEEP
    };
    
    if (rq->bRequest == 0) { // get device info
        usbMsgPtr = replyBuffer;
        return 5;
      
    } else if (rq->bRequest == 1) { // write page
        writeLength = rq->wValue.word;
        currentAddress = rq->wIndex.word;
        return USB_NO_MSG; // magical? IDK - USBaspLoader-tiny85 returns this and it works so whatever.
        
    } else { // exit bootloader
#if BOOTLOADER_CAN_EXIT
        fireEvent(EVENT_EXIT_BOOTLOADER);
#endif
    }
    
    return 0;
}


// read in a page over usb, and write it in to the flash write buffer
static uchar usbFunctionWrite(uchar *data, uchar length) {
    writeLength -= length;
    
    do {
        // remember vectors or the tinyvector table 
        if (currentAddress == RESET_VECTOR_OFFSET * 2) {
            vectorTemp[0] = *(short *)data;
        }
        
        if (currentAddress == USBPLUS_VECTOR_OFFSET * 2) {
            vectorTemp[1] = *(short *)data;
        }
        
        // make sure we don't write over the bootloader!
        if (currentAddress >= BOOTLOADER_ADDRESS - 6) {
            __boot_page_fill_clear();
            break;
        }
        
        writeWordToPageBuffer(*(uint16_t *) data);
        data += 2; // advance data pointer
        length -= 2;
    } while(length);
    
    // TODO: Isn't this always last?
    // if we have now reached another page boundary, we're done
    uchar isLast = (writeLength == 0);
    if (isLast) fireEvent(EVENT_WRITE_PAGE); // ask runloop to write our page
    
    return isLast; // let vusb know we're done with this request
}

/* ------------------------------------------------------------------------ */

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

static inline void tiny85FlashInit(void) {
    // check for erased first page (no bootloader interrupt vectors), add vectors if missing
    // this needs to happen for usb communication to work later - essential to first run after bootloader
    // being installed
    if(pgm_read_word(RESET_VECTOR_OFFSET * 2) != 0xC000 + (BOOTLOADER_ADDRESS/2) - 1 ||
            pgm_read_word(USBPLUS_VECTOR_OFFSET * 2) != 0xC000 + (BOOTLOADER_ADDRESS/2) - 1) {

        fillFlashWithVectors();
    }

    // TODO: necessary to reset currentAddress?
    currentAddress = 0;
}

static inline void tiny85FlashWrites(void) {
    _delay_ms(2); // TODO: why is this here? - it just adds pointless two level deep loops seems like?
    // write page to flash, interrupts will be disabled for > 4.5ms including erase
    
    if (currentAddress % SPM_PAGESIZE) {
        fillFlashWithVectors();
    } else {
        writeFlashPage();
    }
}

static inline __attribute__((noreturn)) void leaveBootloader(void) {
    //DBG1(0x01, 0, 0);
    bootLoaderExit();
    cli();
    USB_INTR_ENABLE = 0;
    USB_INTR_CFG = 0;       /* also reset config bits */

    // make sure remainder of flash is erased and write checksum and application reset vectors
    if (didWriteSomething) {
        while (currentAddress < BOOTLOADER_ADDRESS) {
            fillFlashWithVectors();
        }
    }

    // clear magic word from bottom of stack before jumping to the app
    *(uint8_t*)(RAMEND) = 0x00;
    *(uint8_t*)(RAMEND-1) = 0x00;

    // jump to application reset vector at end of flash
    asm volatile ("rjmp __vectors - 4");
}

int __attribute__((noreturn)) main(void) {
    uint16_t idlePolls = 0;

    /* initialize  */
    wdt_disable();      /* main app may have enabled watchdog */
    tiny85FlashInit();
    currentAddress = 0; // TODO: think about if this is necessary
    bootLoaderInit();
    
    
    if (bootLoaderCondition()){
        initForUsbConnectivity();
        do {
            usbPoll();
            _delay_us(100);
            idlePolls++;
            
            if (events) idlePolls = 0;
            
            // these next two freeze the chip for ~ 4.5ms, breaking usb protocol
            // and usually both of these will activate in the same loop, so host
            // needs to wait > 9ms before next usb request
            if (isEvent(EVENT_PAGE_NEEDS_ERASE)) eraseFlashPage();
            if (isEvent(EVENT_WRITE_PAGE)) tiny85FlashWrites();

#           if BOOTLOADER_CAN_EXIT
                // exit if requested by the programming app, or if we timeout waiting for the pc with a valid app
                if (isEvent(EVENT_EXIT_BOOTLOADER) || AUTO_EXIT_CONDITION()) {
                    _delay_ms(10);
                    break;
                }
#           endif
            
            clearEvents();
            
        } while(bootLoaderCondition());  /* main event loop */
    }
    leaveBootloader();
}

/* ------------------------------------------------------------------------ */
