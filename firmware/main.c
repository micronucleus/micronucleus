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
#include <string.h>

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

// state for uploading process
static uchar state = 0;
#define STATE_UNDEFINED 0
#define STATE_NEW_PAGE 1
#define STATE_CONTINUING_PAGE 2

// becomes 1 when some programming happened
// lets leaveBootloader know if needs to finish up the programming
static uchar didWriteSomething = 0;

static uint16_t         vectorTemp[2];
static addr_t  currentAddress; /* in bytes */


PROGMEM char usbHidReportDescriptor[33] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)

    0x85, 0x01,                    //   REPORT_ID (1)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, 0x02,                    //   REPORT_ID (2)
    0x95, 0x83,                    //   REPORT_COUNT (131)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};


/* ------------------------------------------------------------------------ */

// TODO: inline these?
static void eraseFlashPage(void) {
    cli();
    boot_page_erase(currentAddress - 2);
    boot_spm_busy_wait();
    sei();
}

static void writeFlashPage(void) {
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
	if (pgm_read_word(currentAddress) != data && pgm_read_word(currentAddress) != 0xFFFF) {
        fireEvent(EVENT_PAGE_NEEDS_ERASE);
    }
    
    currentAddress += 2;
}

static void fillFlashWithVectors(void)
{
    int16_t i;

    // fill all or remainder of page with 0xFFFF
    for (i = currentAddress % SPM_PAGESIZE; i < SPM_PAGESIZE; i += 2) {
        writeWordToPageBuffer(0xFFFF);
    }

    writeFlashPage();
}

// #	if HAVE_CHIP_ERASE
// static void eraseApplication(void)
// {
//     // erase all pages starting from end of application section down to page 1 (leaving page 0)
//     currentAddress = BOOTLOADER_ADDRESS - SPM_PAGESIZE;
//     while(currentAddress != 0x0000)
//     {
//         boot_page_erase(currentAddress);
//         boot_spm_busy_wait();
// 
//         currentAddress -= SPM_PAGESIZE;
//     }
// 
//     // erase and load page 0 with vectors
//     fillFlashWithVectors();
// }
// #	endif


static inline __attribute__((noreturn)) void leaveBootloader(void) {
    //DBG1(0x01, 0, 0);
    bootLoaderExit();
    cli();
    USB_INTR_ENABLE = 0;
    USB_INTR_CFG = 0;       /* also reset config bits */
    
	// make sure remainder of flash is erased and write checksum and application reset vectors
	if (didWriteSomething) {
    //if(appWriteComplete) {
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

/* ------------------------------------------------------------------------ */

uchar usbFunctionSetup(uchar data[8]) {
   usbRequest_t *rq = (void *)data;
   static uchar replyBuffer[7] = { // TODO: Adjust this buffer size when trimming off those two useless bytes
      1, // report ID
      SPM_PAGESIZE & 0xff,
      SPM_PAGESIZE >> 8, // also completely useless on tiny85's - they'll never have more than 64 byte pagesize
      ((uint)PROGMEM_SIZE) & 0xff,
      (((uint)PROGMEM_SIZE) >> 8) & 0xff,
      0,
      0 // TODO: remove these last ones that don't do anything for these small chips ?
   };

   if (rq->bRequest == USBRQ_HID_SET_REPORT) {
      if (rq->wValue.bytes[0] == 2) {
         //offset = 0;
         state = STATE_NEW_PAGE;
         return USB_NO_MSG;
      }
#if BOOTLOADER_CAN_EXIT
      else {
         fireEvent(EVENT_EXIT_BOOTLOADER);
      }
#endif
   } else if (rq->bRequest == USBRQ_HID_GET_REPORT) {
      usbMsgPtr = replyBuffer;
      return 7; // TODO: Adjust this to match replyBuffer's size after trimming two useless bytes
   }
   return 0;
}
// 
// uchar usbFunctionSetup(uchar data[8]) {
//     usbRequest_t    *rq = (void *)data;
//     uchar           len = 0;
//     static uchar    replyBuffer[4];
//     
// 
//     usbMsgPtr = replyBuffer;
//     if (rq->bRequest == USBASP_FUNC_TRANSMIT) {   /* emulate parts of ISP protocol */
//         uchar rval = 0;
//         usbWord_t address;
//         address.bytes[1] = rq->wValue.bytes[1];
//         address.bytes[0] = rq->wIndex.bytes[0];
//         if(rq->wValue.bytes[0] == 0x30){        /* read signature */
//             rval = rq->wIndex.bytes[0] & 3;
//             rval = signatureBytes[rval];
// #if HAVE_EEPROM_BYTE_ACCESS
//         }else if(rq->wValue.bytes[0] == 0xa0){  /* read EEPROM byte */
//             rval = eeprom_read_byte((void *)address.word);
//         }else if(rq->wValue.bytes[0] == 0xc0){  /* write EEPROM byte */
//             eeprom_write_byte((void *)address.word, rq->wIndex.bytes[1]);
// #endif
// #if HAVE_CHIP_ERASE
//         }else if(rq->wValue.bytes[0] == 0xac && rq->wValue.bytes[1] == 0x80){  /* chip erase */
// #	ifdef TINY85MODE
//             eraseRequested = 1;
// #	else
//             addr_t addr;
//             for(addr = 0; addr < FLASHEND + 1 - 2048; addr += SPM_PAGESIZE) {
//                 /* wait and erase page */
//                 //DBG1(0x33, 0, 0);
// #   	ifndef NO_FLASH_WRITE
//                 boot_spm_busy_wait();
//                 cli();
//                 boot_page_erase(addr);
//                 sei();
// #   	endif
//             }
// #	endif
// #endif
//         }else{
//             /* ignore all others, return default value == 0 */
//         }
//         replyBuffer[3] = rval;
//         len = 4;
//     }else if(rq->bRequest == USBASP_FUNC_ENABLEPROG){
//         /* replyBuffer[0] = 0; is never touched and thus always 0 which means success */
//         len = 1;
//     }else if(rq->bRequest >= USBASP_FUNC_READFLASH && rq->bRequest <= USBASP_FUNC_SETLONGADDRESS){
//         currentAddress.w[0] = rq->wValue.word;
//         if(rq->bRequest == USBASP_FUNC_SETLONGADDRESS){
// #if (FLASHEND) > 0xffff
//             currentAddress.w[1] = rq->wIndex.word;
// #endif
//         }else{
//             bytesRemaining = rq->wLength.bytes[0];
//             /* if(rq->bRequest == USBASP_FUNC_WRITEFLASH) only evaluated during writeFlash anyway */
//             isLastPage = rq->wIndex.bytes[1] & 0x02;
// #if HAVE_EEPROM_PAGED_ACCESS
//             currentRequest = rq->bRequest;
// #endif
//             len = 0xff; /* hand over to usbFunctionRead() / usbFunctionWrite() */
//         }
// #if BOOTLOADER_CAN_EXIT
//     }else if(rq->bRequest == USBASP_FUNC_DISCONNECT){
//         requestBootLoaderExit = 1;      /* allow proper shutdown/close of connection */
// #endif
//     }else{
//         /* ignore: USBASP_FUNC_CONNECT */
//     }
//     return len;
// }

// uchar usbFunctionWrite(uchar *data, uchar len)
// {
//     uchar   isLast;
//     
//     //DBG1(0x31, (void *)&currentAddress.l, 4);
//     if(len > bytesRemaining)
//         len = bytesRemaining;
//     bytesRemaining -= len;
//     isLast = bytesRemaining == 0;
// #if HAVE_EEPROM_PAGED_ACCESS
//     if(currentRequest >= USBASP_FUNC_READEEPROM){
//         uchar i;
//         for(i = 0; i < len; i++){
//             eeprom_write_byte((void *)(currentAddress.w[0]++), *data++);
//         }
//     }else {
// #endif
//         uchar i;
//         for(i = 0; i < len;){
// //#ifdef TINY85MODE
// //#if 1
//             if(currentAddress == RESET_VECTOR_OFFSET * 2)
//             {
//                 vectorTemp[0] = *(short *)data;
//             }
//             if(currentAddress == USBPLUS_VECTOR_OFFSET * 2)
//             {
//                 vectorTemp[1] = *(short *)data;
//             }
// // #else
// //             if(currentAddress == RESET_VECTOR_OFFSET * 2 || currentAddress == USBPLUS_VECTOR_OFFSET * 2)
// //             {
// //                 vectorTemp[currentAddress ? 1:0] = *(short *)data;
// //             }
// // #endif
// 
//             i += 2;
//             //DBG1(0x32, 0, 0);
// #ifdef TINY85MODE
//             if(currentAddress >= BOOTLOADER_ADDRESS - 6)
//             {
//                 // stop writing data to flash if the application is too big, and clear any leftover data in the page buffer
//                 __boot_page_fill_clear();
//                 return isLast;
//             }
// 
//             writeWordToPageBuffer(*(short *)data);
// #else
// 			cli();
//             boot_page_fill(currentAddress, *(short *)data);
//             sei();
//             currentAddress += 2;
// #endif
//             data += 2;
//             /* write page when we cross page boundary or we have the last partial page */
//             if((currentAddress.w[0] & (SPM_PAGESIZE - 1)) == 0 || (isLast && i >= len && isLastPage)){
//                 //DBG1(0x34, 0, 0);
// #ifdef TINY85MODE
//                 flashPageLoaded = 1;
// #else
// #	ifndef NO_FLASH_WRITE
//                 cli();
//                 boot_page_write(currentAddress - 2);
//                 sei();
//                 boot_spm_busy_wait();
//                 cli();
//                 boot_rww_enable();
//                 sei();
// #	endif
// #endif
//             }
//         }
// 
//     return isLast;
// }

// read in a page over usb, and write it in to the flash write buffer
uchar usbFunctionWrite(uchar *data, uchar length) {
    union {
        addr_t  l;
        uint    s[sizeof(addr_t)/2];
        uchar   c[sizeof(addr_t)];
    } address;
    
    // if we are the first functionWrite for this page, update our write address
    if (state == STATE_NEW_PAGE) {
        address.l = currentAddress;
        
        //DBG1(0x30, data, 3);
        address.c[0] = data[1];
        address.c[1] = data[2];
        data += 4;
        length -= 4;
        
        currentAddress = address.l;
        
        state = STATE_CONTINUING_PAGE;
    }
    
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
        currentAddress += 2; // advance progmem address
        data += 2; // advance data pointer
        length -= 2;
    } while(length);
    
    // if we have now reached another page boundary, we're done
    uchar isLast = !(currentAddress % SPM_PAGESIZE == 0);
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

static inline void tiny85FlashWrites(void) {
    _delay_ms(2); // TODO: why is this here?
    // write page to flash, interrupts will be disabled for > 4.5ms including erase
    
    if (currentAddress % SPM_PAGESIZE) {
        fillFlashWithVectors();
    } else {
        writeFlashPage();
    }
}

int __attribute__((noreturn)) main(void) {
    uint16_t idlePolls = 0;

    /* initialize  */
    wdt_disable();      /* main app may have enabled watchdog */
    //tiny85FlashInit();
    currentAddress = 0; // TODO: think about if this is necessary
    bootLoaderInit();
    //odDebugInit();
    ////DBG1(0x00, 0, 0);


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
