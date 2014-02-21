/* 
 * Project: Micronucleus -  v2.0
 *
 * Micronucleus V2.0             (c) 2014 Tim Bo"scke - cpldcpu@gmail.com
 *                               (c) 2014 Shay Green
 * Original Micronucleus         (c) 2012 Jenna Fox
 *
 * Based on USBaspLoader-tiny85  (c) 2012 Louis Beaudoin
 * Based on USBaspLoader         (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 *
 * License: GNU GPL v2 (see License.txt)
 */
 
#define MICRONUCLEUS_VERSION_MAJOR 2
#define MICRONUCLEUS_VERSION_MINOR 0

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <util/delay.h>

#include "bootloaderconfig.h"
#include "usbdrv/usbdrv.c"

// how many milliseconds should host wait till it sends another erase or write?
// needs to be above 4.5 (and a whole integer) as avr freezes for 4.5ms
#define MICRONUCLEUS_WRITE_SLEEP 5

// verify the bootloader address aligns with page size
#if BOOTLOADER_ADDRESS % SPM_PAGESIZE != 0
  #error "BOOTLOADER_ADDRESS in makefile must be a multiple of chip's pagesize"
#endif

#if SPM_PAGESIZE>256
  #error "Micronucleus only supports pagesizes up to 256 bytes"
#endif

// Device configuration reply
// Length: 4 bytes
//   Byte 0:  User program memory size, high byte
//   Byte 1:  User program memory size, low byte   
//   Byte 2:  Flash Pagesize in bytes
//   Byte 3:  Page write timing in ms

PROGMEM const uint8_t configurationReply[4] = {
  (((uint16_t)PROGMEM_SIZE) >> 8) & 0xff,
  ((uint16_t)PROGMEM_SIZE) & 0xff,
  SPM_PAGESIZE,
  MICRONUCLEUS_WRITE_SLEEP
};  

  typedef union {
    uint16_t w;
    uint8_t b[2];
  } uint16_union_t;
  
#if OSCCAL_RESTORE
  register uint8_t      osccal_default  asm("r2");
#endif 

register uint16_union_t currentAddress  asm("r4");  // r4/r5 current progmem address, used for erasing and writing 
register uint16_union_t idlePolls       asm("r6");  // r6/r7 idlecounter

// command system schedules functions to run in the main loop
enum {
  cmd_local_nop=0, 
  cmd_device_info=0,
  cmd_transfer_page=1,
  cmd_erase_application=2,
  cmd_write_data=3,
  cmd_exit=4,
  cmd_write_page=64,  // internal commands start at 64
};
register uint8_t        command         asm("r3");  // bind command to r3 

// Definition of sei and cli without memory barrier keyword to prevent reloading of memory variables
#define sei() asm volatile("sei")
#define cli() asm volatile("cli")
#define nop() asm volatile("nop")

// Use the old delay routines without NOP padding. This saves memory.
#define __DELAY_BACKWARD_COMPATIBLE__   

/* ------------------------------------------------------------------------ */
static inline void eraseApplication(void);
static void writeFlashPage(void);
static void writeWordToPageBuffer(uint16_t data);
static uint8_t usbFunctionSetup(uint8_t data[8]);
static inline void leaveBootloader(void);

// This function is never called, it is just here to suppress a compiler warning.
USB_PUBLIC usbMsgLen_t usbFunctionDescriptor(struct usbRequest *rq) { return 0; }

// clear memory which stores data to be written by next writeFlashPage call
#define __boot_page_fill_clear()             \
(__extension__({                             \
  __asm__ __volatile__                       \
  (                                          \
    "out %0, %1\n\t"                         \
    "spm\n\t"                                \
    :                                        \
    : "i" (_SFR_IO_ADDR(__SPM_REG)),        \
      "r" ((uint8_t)(__BOOT_PAGE_FILL | (1 << CTPB)))     \
  );                                           \
}))

// erase any existing application and write in jumps for usb interrupt and reset to bootloader
//  - Because flash can be erased once and programmed several times, we can write the bootloader
//  - vectors in now, and write in the application stuff around them later.
//  - if vectors weren't written back in immediately, usb would fail.
static inline void eraseApplication(void) {
  // erase all pages until bootloader, in reverse order (so our vectors stay in place for as long as possible)
  // while the vectors don't matter for usb comms as interrupts are disabled during erase, it's important
  // to minimise the chance of leaving the device in a state where the bootloader wont run, if there's power failure
  // during upload

  uint16_t ptr = BOOTLOADER_ADDRESS;

  while (ptr) {
    ptr -= SPM_PAGESIZE;        
    boot_page_erase(ptr);
  }
 
  // clear page buffer as a precaution before filling the buffer in case
  // a previous write operation failed and there is still something in the buffer.
  __boot_page_fill_clear(); 
  
  // Write reset vector into first page.
  currentAddress.w = 0; 
  writeWordToPageBuffer(0xffff);
#if BOOTLOADER_ADDRESS >= 8192 
  writeWordToPageBuffer(0xffff);  // far jmp
#endif
  command=cmd_write_page;
}

// simply write currently stored page in to already erased flash memory
static inline void writeFlashPage(void) {
  if (currentAddress.w - 2 <BOOTLOADER_ADDRESS)
      boot_page_write(currentAddress.w - 2);   // will halt CPU, no waiting required
}

// write a word into the page buffer, doing interrupt table modifications where they're required
static void writeWordToPageBuffer(uint16_t data) {
  
  // Patch the bootloader reset vector into the main vectortable to ensure
  // the device can not be bricked.
  // Saving user-reset-vector is done in the host tool, starting with
  // firmware V2
#if BOOTLOADER_ADDRESS < 8192
  // rjmp
  if (currentAddress.w == RESET_VECTOR_OFFSET * 2) {
    data = 0xC000 + (BOOTLOADER_ADDRESS/2) - 1;
  }
#else
  // far jmp
  if (currentAddress.w == RESET_VECTOR_OFFSET * 2) {
    data = 0x940c;
  } else if (currentAddress.w == (RESET_VECTOR_OFFSET +1 ) * 2) {
    data = (BOOTLOADER_ADDRESS/2);
  }    
#endif

#if (!OSCCAL_RESTORE) && OSCCAL_16_5MHz   
   if (currentAddress.w == BOOTLOADER_ADDRESS - TINYVECTOR_OSCCAL_OFFSET) {
      data = OSCCAL;
   }     
#endif
  
  boot_page_fill(currentAddress.w, data);
  currentAddress.w += 2;
}

/* ------------------------------------------------------------------------ */
static uint8_t usbFunctionSetup(uint8_t data[8]) {
  usbRequest_t *rq = (void *)data;
 
  idlePolls.b[1]=0; // reset idle polls when we get usb traffic

  if (rq->bRequest == cmd_device_info) { // get device info
    usbMsgPtr = (usbMsgPtr_t)configurationReply;
    return sizeof(configurationReply);      
  } else if (rq->bRequest == cmd_transfer_page) { // initialize write page
      currentAddress.w = rq->wIndex.word;     
    } else if (rq->bRequest == cmd_write_data) { // Write data
      writeWordToPageBuffer(rq->wValue.word);
      writeWordToPageBuffer(rq->wIndex.word);
      if ((currentAddress.b[0] % SPM_PAGESIZE) == 0)
          command=cmd_write_page; // ask runloop to write our page       
  } else {
    // Handle cmd_erase_application and cmd_exit
    command=rq->bRequest;
  }
  return 0;
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
  _delay_ms(300);  
  usbDeviceConnect();

  usbInit();    // Initialize INT settings after reconnect
}

/* ------------------------------------------------------------------------ */
// reset system to a normal state and launch user program
static void leaveBootloader(void) __attribute__((__noreturn__));
static inline void leaveBootloader(void) {
 
  bootLoaderExit();

  usbDeviceDisconnect();  /* Disconnect micronucleus */

  USB_INTR_ENABLE = 0;
  USB_INTR_CFG = 0;       /* also reset config bits */

  #if OSCCAL_RESTORE
    OSCCAL=osccal_default;
    nop(); // NOP to avoid CPU hickup during oscillator stabilization
  #elif OSCCAL_16_5MHz   
    // adjust clock to previous calibration value, so user program always starts with same calibration
    // as when it was uploaded originally
    unsigned char stored_osc_calibration = pgm_read_byte(BOOTLOADER_ADDRESS - TINYVECTOR_OSCCAL_OFFSET);
    if (stored_osc_calibration != 0xFF) {
      OSCCAL=stored_osc_calibration;
      nop();
    }
  #endif
  
 asm volatile ("rjmp __vectors - 4"); // jump to application reset vector at end of flash
  
 for (;;); // Make sure function does not return to help compiler optimize
}

void USB_INTR_VECTOR(void);
int main(void) {
  bootLoaderInit();
	  
  if (bootLoaderStartCondition()||(pgm_read_byte(BOOTLOADER_ADDRESS - TINYVECTOR_RESET_OFFSET + 1)==0xff)) {
  
    initHardware();        
    LED_INIT();

    if (AUTO_EXIT_NO_USB_MS>0) {
      idlePolls.b[1]=((AUTO_EXIT_MS-AUTO_EXIT_NO_USB_MS)/5)>>8;
    } else {
      idlePolls.b[1]=0;
    }
    
    command=cmd_local_nop;     
    
    do {
      // 15 clockcycles per loop.     
      // adjust fastctr for 5ms timeout
      
      uint16_t fastctr=(uint16_t)(F_CPU/(1000.0f*15.0f/5.0f));
      uint8_t resetctr=20;
  
      do {        
        if ((USBIN & USBMASK) !=0) resetctr=20;
        
        if (!--resetctr) { // reset encountered
           usbNewDeviceAddr = 0;   // bits from the reset handling of usbpoll()
           usbDeviceAddr = 0;
#if (OSCCAL_HAVE_XTAL == 0)           
           calibrateOscillatorASM();   
#endif           
        }
        
        if (USB_INTR_PENDING & (1<<USB_INTR_PENDING_BIT)) {
          USB_INTR_VECTOR();  // clears INT_PENDING (See se0: in asmcommon.inc)
          break;
        }
        
      } while(--fastctr);     
 
      // commands are only evaluated after next USB transmission or after 5 ms passed
      if (command==cmd_erase_application) 
        eraseApplication();
      // Attention: eraseApplication will set command=cmd_write_page!
      if (command==cmd_write_page) 
        writeFlashPage();          
      if (command==cmd_exit) {
        if (!fastctr) break;  // Only exit after 5 ms timeout     
      } else {
        command=cmd_local_nop;     
      }  
 
      {
      // This is usbpoll() minus reset logic and double buffering
        int8_t  len;
        len = usbRxLen - 3;
        if(len >= 0){
            usbProcessRx(usbRxBuf + 1, len); // only single buffer due to in-order processing
            usbRxLen = 0;       /* mark rx buffer as available */
        }
        if(usbTxLen & 0x10){    /* transmit system idle */
            if(usbMsgLen != USB_NO_MSG){    /* transmit data pending? */
                usbBuildTxBlock();
            }
        }
      }
      
      idlePolls.w++;

      // Try to execute program when bootloader times out      
      if (AUTO_EXIT_MS&&(idlePolls.w==(AUTO_EXIT_MS/5))) {
         if (pgm_read_byte(BOOTLOADER_ADDRESS - TINYVECTOR_RESET_OFFSET + 1)!=0xff)  break;
      }
      
      LED_MACRO( idlePolls.b[0] );   

       // Test whether another interrupt occurred during the processing of USBpoll and commands.
       // If yes, we missed a data packet on the bus. This is not a big issue, since
       // USB seems to allow time-out of up the two packets. 
       // The most critical situation occurs when a PID IN packet is missed due to
       // it's short length. Each packet + timeout takes around 45µs, meaning that
       // usbpoll must take less than 90µs or resyncing is not possible.
       // To avoid synchronizing of the interrupt routine, we must not call it while
       // a packet is transmitted. Therefore we have to wait until the bus is idle again.
       //
       // Just waiting for EOP (SE0) or no activity for 6 bus cycles is not enough,
       // as the host may have been sending a multi-packet transmission (eg. OUT or SETUP)
       // In that case we may resynch within a transmission, causing errors.
       //
       // A safer way is to wait until the bus was idle for the time it takes to send
       // an ACK packet by the client (10.5µs on D+) but not as long as bus
       // time out (12µs)
       //
       
       if (USB_INTR_PENDING & (1<<USB_INTR_PENDING_BIT))  // Usbpoll() collided with data packet
       {        
          uint8_t ctr;
         
          // loop takes 5 cycles
          asm volatile(      
          "         ldi  %0,%1 \n\t"        
          "loop%=:  sbic %2,%3  \n\t"        
          "         ldi  %0,%1  \n\t"
          "         subi %0,1   \n\t"        
          "         brne loop%= \n\t"   
          : "=&d" (ctr)
          :  "M" ((uint8_t)(10.0f*(F_CPU/1.0e6f)/5.0f+0.5)), "I" (_SFR_IO_ADDR(USBIN)), "M" (USB_CFG_DPLUS_BIT)
          );       
         USB_INTR_PENDING = 1<<USB_INTR_PENDING_BIT;                   
       }                        
    } while(1);  

    LED_EXIT();
  }
   
  leaveBootloader();
}
/* ------------------------------------------------------------------------ */
