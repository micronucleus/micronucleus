/* 
 * Project: Micronucleus -  v1.11
 * 
 * Original author               (c) 2012 Jenna Fox
 *
 * Optimizations v1.10/v1.11     (c) 2013 Tim Bo"scke - cpldcpu@gmail.com
 *                     v1.11     (c) 2013 Shay Green
 *
 * Based on USBaspLoader-tiny85  (c) 2012 Louis Beaudoin
 * Based on USBaspLoader         (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 *
 * License: GNU GPL v2 (see License.txt)
 */
 
#define MICRONUCLEUS_VERSION_MAJOR 1
#define MICRONUCLEUS_VERSION_MINOR 11
// how many milliseconds should host wait till it sends another erase or write?
// needs to be above 4.5 (and a whole integer) as avr freezes for 4.5ms
#define MICRONUCLEUS_WRITE_SLEEP 8
// Use the old delay routines without NOP padding. This saves memory.
#define __DELAY_BACKWARD_COMPATIBLE__     

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <util/delay.h>

#include "bootloaderconfig.h"



#include "usbdrv/usbdrv.c"

// verify the bootloader address aligns with page size
#if BOOTLOADER_ADDRESS % SPM_PAGESIZE != 0
  #error "BOOTLOADER_ADDRESS in makefile must be a multiple of chip's pagesize"
#endif

#if SPM_PAGESIZE>256
  #error "Micronucleus only supports pagesizes up to 256 bytes"
#endif

// command system schedules functions to run in the main loop
register uint8_t        command         asm("r3");  // bind command to r3 
register uint16_union_t currentAddress  asm("r4");  // r4/r5 current progmem address, used for erasing and writing 
register uint16_union_t idlePolls       asm("r6");  // r6/r7 idlecounter

#if OSCCAL_RESTORE
  register uint8_t      osccal_default  asm("r2");
#endif 

static uint16_t vectorTemp[2]; // remember data to create tinyVector table before BOOTLOADER_ADDRESS

enum {
  cmd_local_nop=0, // also: get device info
  cmd_device_info=0,
  cmd_transfer_page=1,
  cmd_erase_application=2,
  cmd_exit=4,
  cmd_write_page=5,
};

// Definition of sei and cli without memory barrier keyword to prevent reloading of memory variables
#define sei() asm volatile("sei")
#define cli() asm volatile("cli")
#define nop() asm volatile("nop")

/* ------------------------------------------------------------------------ */
static inline void eraseApplication(void);
static void writeFlashPage(void);
static void writeWordToPageBuffer(uint16_t data);
static uint8_t usbFunctionSetup(uint8_t data[8]);
static uint8_t usbFunctionWrite(uint8_t *data, uint8_t length);
static inline void leaveBootloader(void);

// erase any existing application and write in jumps for usb interrupt and reset to bootloader
//  - Because flash can be erased once and programmed several times, we can write the bootloader
//  - vectors in now, and write in the application stuff around them later.
//  - if vectors weren't written back in immediately, usb would fail.
static inline void eraseApplication(void) {
  // erase all pages until bootloader, in reverse order (so our vectors stay in place for as long as possible)
  // while the vectors don't matter for usb comms as interrupts are disabled during erase, it's important
  // to minimise the chance of leaving the device in a state where the bootloader wont run, if there's power failure
  // during upload
  
  uint8_t i;
  uint16_t ptr = BOOTLOADER_ADDRESS;
  cli();

  while (ptr) {
    ptr -= SPM_PAGESIZE;        
    boot_page_erase(ptr);
  }
    
	currentAddress.w = 0;
  for (i=0; i<8; i++) writeWordToPageBuffer(0xFFFF);  // Write first 8 words to fill in vectors.
  writeFlashPage();  // enables interrupts
}

// simply write currently stored page in to already erased flash memory
static void writeFlashPage(void) {
  cli();
  boot_page_write(currentAddress.w - 2);   // will halt CPU, no waiting required
  sei();
}

// clear memory which stores data to be written by next writeFlashPage call
#define __boot_page_fill_clear()             \
(__extension__({                             \
  __asm__ __volatile__                       \
  (                                          \
    "sts %0, %1\n\t"                         \
    "spm\n\t"                                \
    :                                        \
    : "i" (_SFR_MEM_ADDR(__SPM_REG)),        \
      "r" ((uint8_t)(__BOOT_PAGE_FILL | (1 << CTPB)))     \
  );                                           \
}))

// write a word in to the page buffer, doing interrupt table modifications where they're required
static void writeWordToPageBuffer(uint16_t data) {
  uint8_t previous_sreg;
  
  // first two interrupt vectors get replaced with a jump to the bootloader's vector table
  // remember vectors or the tinyvector table 
    if (currentAddress.w == RESET_VECTOR_OFFSET * 2) {
      vectorTemp[0] = data;
      data = 0xC000 + (BOOTLOADER_ADDRESS/2) - 1;
    }
    
    if (currentAddress.w == USBPLUS_VECTOR_OFFSET * 2) {
      vectorTemp[1] = data;
      data = 0xC000 + (BOOTLOADER_ADDRESS/2) - 1;
    }
    
  // at end of page just before bootloader, write in tinyVector table
  // see http://embedded-creations.com/projects/attiny85-usb-bootloader-overview/avr-jtag-programmer/
  // for info on how the tiny vector table works
  if (currentAddress.w == BOOTLOADER_ADDRESS - TINYVECTOR_RESET_OFFSET) {
      data = vectorTemp[0] + ((FLASHEND + 1) - BOOTLOADER_ADDRESS)/2 + 2 + RESET_VECTOR_OFFSET;
  } else if (currentAddress.w == BOOTLOADER_ADDRESS - TINYVECTOR_USBPLUS_OFFSET) {
      data = vectorTemp[1] + ((FLASHEND + 1) - BOOTLOADER_ADDRESS)/2 + 1 + USBPLUS_VECTOR_OFFSET;
#if (!OSCCAL_RESTORE) && OSCCAL_16_5MHz   
  } else if (currentAddress.w == BOOTLOADER_ADDRESS - TINYVECTOR_OSCCAL_OFFSET) {
      data = OSCCAL;
#endif		
  }

  previous_sreg=SREG;    
  cli(); // ensure interrupts are disabled
  
  boot_page_fill(currentAddress.w, data);
  
  // increment progmem address by one word
  currentAddress.w += 2;
  SREG=previous_sreg;
}

// This function is never called, it is just here to suppress a compiler warning.
USB_PUBLIC usbMsgLen_t usbFunctionDescriptor(struct usbRequest *rq) { return 0; }

/* ------------------------------------------------------------------------ */
static uint8_t usbFunctionSetup(uint8_t data[8]) {
  usbRequest_t *rq = (void *)data;

  static uint8_t replyBuffer[4] = {
    (((uint16_t)PROGMEM_SIZE) >> 8) & 0xff,
    ((uint16_t)PROGMEM_SIZE) & 0xff,
    SPM_PAGESIZE,
    MICRONUCLEUS_WRITE_SLEEP
  };

  idlePolls.b[1]=0; // reset idle polls when we get usb traffic

  if (rq->bRequest == cmd_device_info) { // get device info
    usbMsgPtr = replyBuffer;
    return 4;      
  } else if (rq->bRequest == cmd_transfer_page) { // transfer page  
    // clear page buffer as a precaution before filling the buffer in case 
    // a previous write operation failed and there is still something in the buffer.
    __boot_page_fill_clear();
    currentAddress.w = rq->wIndex.word;        
    return USB_NO_MSG; // hands off work to usbFunctionWrite
  } else {
    // Handle cmd_erase_application and cmd_exit
    command=rq->bRequest;
    return 0;
  }
}

// read in a page over usb, and write it in to the flash write buffer
static uint8_t usbFunctionWrite(uint8_t *data, uint8_t length) {
  do {     
    // make sure we don't write over the bootloader!
    if (currentAddress.w >= BOOTLOADER_ADDRESS) break;
    
    writeWordToPageBuffer(*(uint16_t *) data);
    data += 2; // advance data pointer
    length -= 2;
  } while(length);
  
  // if we have now reached another page boundary, we're done
  uint8_t isLast = ((currentAddress.b[0] % SPM_PAGESIZE) == 0);
  if (isLast) command=cmd_write_page; // ask runloop to write our page
  
  return isLast; // let V-USB know we're done with this request
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

static void initHardware (void)
{
  // Disable watchdog and set timeout to maximum in case the WDT is fused on 
  MCUSR=0;    
  WDTCR = 1<<WDCE | 1<<WDE;
  WDTCR = 1<<WDP2 | 1<<WDP1 | 1<<WDP0; 

  /* initialize  */
  #if OSCCAL_RESTORE
    osccal_default = OSCCAL;
  #endif
    
  usbDeviceDisconnect();  /* do this while interrupts are disabled */
  _delay_ms(500);  
  usbDeviceConnect();
  usbInit();    // Initialize INT settings after reconnect

  sei();        
}

/* ------------------------------------------------------------------------ */
// reset system to a normal state and launch user program
static void leaveBootloader(void) __attribute__((__noreturn__));
static inline void leaveBootloader(void) {
  
  bootLoaderExit();
  cli();
	usbDeviceDisconnect();  /* Disconnect micronucleus */

  USB_INTR_ENABLE = 0;
  USB_INTR_CFG = 0;       /* also reset config bits */

  // clear magic word from bottom of stack before jumping to the app
  *(uint8_t*)(RAMEND) = 0x00; // A single write is sufficient to invalidate magic word
    
  #if OSCCAL_RESTORE
    OSCCAL=osccal_default;
    nop();	// NOP to avoid CPU hickup during oscillator stabilization
  #elif OSCCAL_16_5MHz   
    // adjust clock to previous calibration value, so user program always starts with same calibration
    // as when it was uploaded originally
    unsigned char stored_osc_calibration = pgm_read_byte(BOOTLOADER_ADDRESS - TINYVECTOR_OSCCAL_OFFSET);
    if (stored_osc_calibration != 0xFF && stored_osc_calibration != 0x00) {
      OSCCAL=stored_osc_calibration;
      nop();
    }
  #endif
  
  asm volatile ("rjmp __vectors - 4"); // jump to application reset vector at end of flash
  
  for (;;); // Make sure function does not return to help compiler optimize
}

int main(void) {
    
  bootLoaderInit();
	
  if (bootLoaderStartCondition()||(pgm_read_byte(BOOTLOADER_ADDRESS - TINYVECTOR_RESET_OFFSET + 1)==0xff)) {
  
    initHardware();        
    LED_INIT();

    if (AUTO_EXIT_NO_USB_MS>0) {
      idlePolls.b[1]=((AUTO_EXIT_MS-AUTO_EXIT_NO_USB_MS) * 10UL)>>8;
    } else {
      idlePolls.b[1]=0;
    }
    
    do {
      _delay_us(100);
      wdt_reset();   // Only necessary if WDT is fused on
      
      command=cmd_local_nop;
      usbPoll();
     
      idlePolls.w++;
      
      // Try to execute program if bootloader exit condition is met
      if (AUTO_EXIT_MS&&(idlePolls.w==AUTO_EXIT_MS*10L)) command=cmd_exit;
 
      LED_MACRO( idlePolls.b[1] );

      // Wait for USB traffic to finish before a blocking event is executed
      // All events will render the MCU unresponsive to USB traffic for a while.
      if (command!=cmd_local_nop) _delay_ms(2);
 
      if (command==cmd_erase_application) 
        eraseApplication();
      else if (command==cmd_write_page) 
        writeFlashPage();
       
      /* main event loop runs as long as no problem is uploaded or existing program is not executed */                           
    } while((command!=cmd_exit)||(pgm_read_byte(BOOTLOADER_ADDRESS - TINYVECTOR_RESET_OFFSET + 1)==0xff));  

    LED_EXIT();
  }
    
  leaveBootloader();
}
/* ------------------------------------------------------------------------ */
