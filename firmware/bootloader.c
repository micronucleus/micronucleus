/* Name: main.c
 * Project: Micronucleus
 * Author: Jenna Fox
 * Creation Date: 2007-12-08
 * Tabsize: 4
 * Copyright: (c) 2012 Jenna Fox
 * Portions Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH (USBaspLoader)
 * Portions Copyright: (c) 2012 Louis Beaudoin (USBaspLoader-tiny85)
 * License: GNU GPL v2 (see License.txt)
 */

#define MICRONUCLEUS_VERSION_MAJOR 1
#define MICRONUCLEUS_VERSION_MINOR 6
// how many milliseconds should host wait till it sends another erase or write?
// needs to be above 4.5 (and a whole integer) as avr freezes for 4.5ms
#define MICRONUCLEUS_WRITE_SLEEP 8


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <util/delay.h>

static void leaveBootloader() __attribute__((__noreturn__));

#include "chipid.h"
#include <hardware.h>

/*
 * The RC oscilator can only be used with 16.5 and 12.8 MHz and
 * only when using the RC oscilator any OSCCAL manipulations are
 * necessary.
 */
#if (F_CPU != 16500000) && (F_CPU != 12800000)
#  ifndef WITH_CRYSTAL
#    define WITH_CRYSTAL 1
#  endif
#endif

#ifndef MICRONUCLEUS_WIRING
#  define MICRONUCLEUS_WIRING	255
#endif

#include "bootloaderconfig.h"
#include "usbdrv/usbdrv.c"

#ifndef WITH_CRYSTAL
#    include <libs-device/osccal.c>
#else
#  undef RESTORE_OSCCAL
#endif

#ifndef FLASH_SIZE
#  define FLASH_SIZE (FLASHEND + 1)
#endif
#ifndef FLASH_WORDS
#  define FLASH_WORDS	(FLASH_SIZE / 2)
#endif
#ifndef VECTOR_WORDS
#  define VECTOR_WORDS	(VECTOR_SIZE / 2)
#endif


/* ------------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------------ */

/*
 * Initialize stack and the zero register.
 *
 * This is essentially the same code as it would be insetrted when linking
 * against the normal crt*.o files.  This is needed because of the
 * "-nostartfiles" flag used to get rid of the interrupt table.
 *
 * This also takes care of pushing the magic "BOOT" indicator onto the stack.
 */
void __initialize_cpu(void) __attribute__ ((naked)) __attribute__ ((section (".init2")));
void __initialize_cpu(void)
{

	asm volatile ( "clr __zero_reg__" );				// r1 set to 0
	asm volatile ( "out __SREG__, r1" );				// clear status register
	asm volatile ( "ldi r28, %0" :: "M" (RAMEND & 0xFF) );		// Initilize stack pointer.
	asm volatile ( "ldi r29, %0" :: "M" ((RAMEND >> 8) & 0xFF) );
	asm volatile ( "out __SP_H__, r29" );
	asm volatile ( "out __SP_L__, r28" );


	/* push the word "B007" at the bottom of the stack (RAMEND - RAMEND-1) */

	asm volatile ("ldi r28, 0xB0" ::);
	asm volatile ("push r28" ::);
	asm volatile ("ldi r28, 0x07" ::);
	asm volatile ("push r28" ::);

}

/*
 * Jump to main.
 *
 * Again, normally the standard crt*.o files would do this, but we dont link
 * them in.
 */

void __jump_to_main(void) __attribute__ ((naked)) __attribute__ ((section (".init9")));
void __jump_to_main(void)
{
	asm volatile ( "rjmp main");                // start main()
}

/*
 * A replacement for the normal interrupt table:
 *
 * Our linker script has a special section ".tinytable" that is placed into the
 * .text section before anything else, just where normaly the vector table is
 * located.
 *
 * - first element is the applications entry point. This is set when we flash
 *   the applications interrupt table, otherwise it contains a jump to the
 *   bootloader init code.
 *
 * - second element is the applications interrupt vector for vUSB.
 *
 * - third element is the place to store the osccal value
 *
 * The above fields are located in the last few words of the programable flash
 * and that page may be erased.
 *
 */

#define APP_RESET_OFFSET 0
void __app_reset(void) __attribute__ ((naked)) __attribute__ ((section (".tinytable"))) __attribute__((__noreturn__));
void __app_reset(void)
{
	asm volatile ( "rjmp __initialize_cpu");
#if VECTOR_SIZE == 4
	asm volatile ( "nop");
#endif
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5))
	__builtin_unreachable();
#endif
}

#define APP_USB_VECTOR_OFFSET (APP_RESET_OFFSET + VECTOR_SIZE)
void __app_usb_vector(void) __attribute__ ((naked)) __attribute__ ((section (".tinytable")));
void __app_usb_vector(void) {
	asm volatile ( "rjmp __initialize_cpu");
#if VECTOR_SIZE == 4
	asm volatile ( "nop");
#endif
}

/*
 * ld does not let us put data into a code section. So this is defined as a function
 * instead, but do not call it !
 */
#define STORED_OSCCAL_OFFSET (APP_USB_VECTOR_OFFSET + VECTOR_SIZE)
void __stored_osccal(void) __attribute__ ((naked)) __attribute__ ((section (".tinytable")));
void __stored_osccal(void) { asm volatile ( "nop" ); }


#define TINY_TABLE_LEN (STORED_OSCCAL_OFFSET + 2)

// verify the bootloader address aligns with page size
#if (BOOTLOADER_ADDRESS + TINY_TABLE_LEN) % SPM_PAGESIZE != 0
#  error "BOOTLOADER_ADDRESS in makefile must be aligned"
#endif

/*
 * Safeguard against erase w/o successive write to the vector table.
 */
#define BOOTLOADER_ENTRY (BOOTLOADER_ADDRESS + TINY_TABLE_LEN)
void __my_reset2(void) __attribute__ ((naked)) __attribute__ ((section (".tinytable")));
void __my_reset2(void) { asm volatile ( "rjmp __initialize_cpu"); }

/*
 * Wrapper for the vUSB interrupt.
 *
 * We look at the two bytes on top of the stack. If those read "B007",
 * we call our own vUSB ISR, otherwise we jump to the applications ISR.
 *
 * The extra latency to get to the bootloader vUSB ISR is 11 cycles.
 *
 * The cost to get to the apps vUSB vector depends on the current stack
 * content, if the final jump to the app is a relative jump and whether
 * "APP_USES_VUSB" is defined.
 *
 * With APP_USES_VUSB
 *   8k devices: best case 10 cycles, worst case 13 cycles
 *  16k devices;           11 cycles,            14 cycles
 *
 * W/o APP_USES_VUSB
 *   8k devices: best case 19 cycles, worst case 23 cycles
 *  16k devices:           20 cycles,            24 cycles
 */

#define BOOTLOADER_INTERRUPT (BOOTLOADER_ENTRY + 2)
void __wrap_vusb_intr(void) __attribute__ ((naked)) __attribute__ ((section (".tinytable")));
void __wrap_vusb_intr(void)
{
	/* Save SREG and YL */
	asm volatile ("push r28");
	asm volatile ("in r28, __SREG__");
	asm volatile ("push r28");

	/* Check our signature above the stack */
	asm volatile ("lds r28, %0" :: "" (RAMEND));		// +  2
	asm volatile ("cpi r28, 0xB0");				// +  1
	asm volatile ("brne jumpToApp");			// +  1   +2

	asm volatile ("lds r28, %0" :: "" (RAMEND - 1));	// +  2
	asm volatile ("cpi r28, 0x07");				// +  1
	asm volatile ("brne jumpToApp");			// +  1	  +2

	/*
	 * Jump to vUSB interrupt vector.
	 *
	 * This jump skips the register save in the ISR so we don't have to
	 * restore SREG and YL. This is a bit hackish though because it makes
	 * assumptions about what exactly gets pushed before "waitForJ". The
	 * 16, 18 and 20MHz versions of the code for example also push YH while
	 * the others do not.
	 */

#if (USB_CFG_CLOCK_KHZ == 16000) || (USB_CFG_CLOCK_KHZ == 18000) || (USB_CFG_CLOCK_KHZ == 20000)
	asm volatile ("push r29");
#endif
	asm volatile ( "clr r28" );				// +  1
	asm volatile ( "rjmp waitForJ" );			// +  2
								// = 11
	/*
	 * Jump to the Applications ISR.
	 *
	 * If we can assume that the application also uses this vector for vUSB,
	 * we can shortcut the register save there too. Otherwise we need to
	 * restore the stack.
	 *
	 */
	asm volatile ( "jumpToApp:" );				// + 5 or + 9 cycles to get here
#ifndef APP_USES_VUSB
#    define APP_VUSB_OFFSET 0
	asm volatile ( "pop r28" );				// +  2
	asm volatile ( "out __SREG__, r28" );			// +  1
	asm volatile ( "pop r28" );				// +  2
#else
#  if (USB_CFG_CLOCK_KHZ == 16000) || (USB_CFG_CLOCK_KHZ == 18000) || (USB_CFG_CLOCK_KHZ == 20000)
#    define APP_VUSB_OFFSET 4
	asm volatile ("push r29");
#  else
#    define APP_VUSB_OFFSET 3
#  endif
	asm volatile ( "clr r28" );				// +  1
#endif
	asm volatile ( "rjmp __app_usb_vector" );		// +  4/5  (2*rjmp or rjmp+jmp)
}


//////// Stuff Bluebie Added

#ifdef AUTO_EXIT_MS
#  if AUTO_EXIT_MS < (MICRONUCLEUS_WRITE_SLEEP * (BOOTLOADER_ADDRESS / SPM_PAGESIZE))
#    warning "AUTO_EXIT_MS is shorter than the time it takes to perform erase function - might affect reliability?"
#    warning "Try increasing AUTO_EXIT_MS if you have stability problems"
#  endif
#endif

// events system schedules functions to run in the main loop
static uchar events = 0; // bitmap of events to run
#define EVENT_ERASE_APPLICATION 1
#define EVENT_WRITE_PAGE 2
#define EVENT_EXECUTE 4

// controls state of events
#define fireEvent(event) events |= (event)
#define isEvent(event)   (events & (event))
#define clearEvents()    events = 0

// length of bytes to write in to flash memory in upcomming usbFunctionWrite calls
//static unsigned char writeLength;

// becomes 1 when some programming happened
// lets leaveBootloader know if needs to finish up the programming
static uchar didWriteSomething = 0;

uint16_t idlePolls = 0; // how long have we been idle?



static uint16_t vectorTemp[2];  // remember data to create tinyVector table before BOOTLOADER_ADDRESS
static uint16_t currentAddress;   // current progmem address, used for erasing and writing


/* ------------------------------------------------------------------------ */
static inline void eraseApplication(void);
static void writeFlashPage(void);
static void writeWordToPageBuffer(uint16_t data);
static void fillFlashWithVectors(void);
static uchar usbFunctionSetup(uchar data[8]);
static uchar usbFunctionWrite(uchar *data, uchar length);
static inline void initForUsbConnectivity(void);
static inline void tiny85FlashWrites(void);
//static inline void tiny85FinishWriting(void);
static inline void leaveBootloader(void);

// erase any existing application and write in jumps for usb interrupt and reset to bootloader
//  - Because flash can be erased once and programmed several times, we can write the bootloader
//  - vectors in now, and write in the application stuff around them later.
//  - if vectors weren't written back in immidately, usb would fail.
static inline void eraseApplication(void)
{
	// erase all pages until bootloader, in reverse order (so our vectors stay in place for as long as possible)
	// while the vectors don't matter for usb comms as interrupts are disabled during erase, it's important
	// to minimise the chance of leaving the device in a state where the bootloader wont run, if there's power failure
	// during upload
	currentAddress = BOOTLOADER_ADDRESS + TINY_TABLE_LEN;
	cli();
	while (currentAddress) {
		currentAddress -= SPM_PAGESIZE;

		boot_page_erase(currentAddress);
		boot_spm_busy_wait();
	}

	fillFlashWithVectors();
	sei();
}

// simply write currently stored page in to already erased flash memory
static void writeFlashPage(void)
{
	didWriteSomething = 1;
	cli();
	boot_page_write(currentAddress - 2);
	boot_spm_busy_wait(); // Wait until the memory is written.
	sei();
}

// clear memory which stores data to be written by next writeFlashPage call
#define __boot_page_fill_clear()   \
	(__extension__({				 \
			       __asm__ __volatile__			    \
			       (					    \
				       "sts %0, %1\n\t"				\
				       "spm\n\t"				\
				       :					\
				       : "i" (_SFR_MEM_ADDR(__SPM_REG)),	\
				       "r" ((uint8_t)(__BOOT_PAGE_FILL | (1 << CTPB)))	   \
			       );					    \
		       }))

inline static uint16_t addr2rjmp(const uint16_t addr, const uint16_t location)
{
	return ( 0xC000 + ((addr - location - 1) & 0xFFF));
}

// write a word in to the page buffer, doing interrupt table modifications where they're required
static void writeWordToPageBuffer(uint16_t data)
{

	/*
	 * Note for 16k devices:
	 *
	 * We assume that the bootloader resides somewhere within the last 4k
	 * of the flash. This assumptions allows us to use the shorter and
	 * faster "rjmp" instructions in the vector table and we don't have to
	 * write the second word in the vector.
	 *
	 * To jump back to the application vectors, we always use absolute
	 * jumps. This maybe costs an additional cycle in vUSB interrupt
	 * latency but saves some code size.
	 *
	 */
	if (currentAddress == (RESET_VECTOR_OFFSET * VECTOR_SIZE)) {
		// I'd like to jump directly to __initialize_cpu, but stupid
		// cpp/c interactions would cost 2 bytes extra
		data = addr2rjmp(BOOTLOADER_ENTRY / 2, RESET_VECTOR_OFFSET * VECTOR_WORDS);
	}
	else if (currentAddress == (USB_INTR_VECTOR_NUM * VECTOR_SIZE)) {
		// same 2 bytes as above, but no-trampoline spares 2 cycles
		// interrupt latency, which I think is worth the expense.
		//data = addr2rjmp((uint16_t)__wrap_vusb_intr, USB_INTR_VECTOR_NUM * VECTOR_WORDS);
		data = addr2rjmp(BOOTLOADER_INTERRUPT / 2, USB_INTR_VECTOR_NUM * VECTOR_WORDS);
	}

	// at end of page just before bootloader, write in tinyVector table
	// see http://embedded-creations.com/projects/attiny85-usb-bootloader-overview/avr-jtag-programmer/
	// for info on how the tiny vector table works
#if VECTOR_SIZE == 2
	else if (currentAddress == BOOTLOADER_ADDRESS + APP_RESET_OFFSET) {
		data = addr2rjmp(vectorTemp[0], (BOOTLOADER_ADDRESS + APP_RESET_OFFSET)/2);
	}
	else if (currentAddress == BOOTLOADER_ADDRESS + APP_USB_VECTOR_OFFSET) {
		data = addr2rjmp(vectorTemp[1], (BOOTLOADER_ADDRESS + APP_USB_VECTOR_OFFSET)/2);
	}
#else
        else if ( currentAddress == BOOTLOADER_ADDRESS + APP_RESET_OFFSET ) {
                data = 0x940c;
        }
        else if ( currentAddress == BOOTLOADER_ADDRESS + APP_RESET_OFFSET + 2 ) {
                data = vectorTemp[0];
        }
        else if ( currentAddress == BOOTLOADER_ADDRESS + APP_USB_VECTOR_OFFSET ) {
                data = 0x940c;
        }
        else if ( currentAddress == BOOTLOADER_ADDRESS + APP_USB_VECTOR_OFFSET + 2 ) {
                data = vectorTemp[1] + APP_USB_VECTOR_OFFSET;
        }
#endif
	else if (currentAddress == BOOTLOADER_ADDRESS + STORED_OSCCAL_OFFSET) {
		data = OSCCAL;
	}

	// clear page buffer as a precaution before filling the buffer on the first page
	// in case the bootloader somehow ran after user program and there was something
	// in the page buffer already
	if (currentAddress == 0x0000) __boot_page_fill_clear();

	cli();
	boot_page_fill(currentAddress, data);
	sei();

	// increment progmem address by one word
	currentAddress += 2;
}

// fills the rest of this page with vectors - interrupt vector or tinyvector tables where needed
static void fillFlashWithVectors(void)
{
	do
		writeWordToPageBuffer(0xFFFF);
	while (currentAddress % SPM_PAGESIZE);

	writeFlashPage();
}

/* ------------------------------------------------------------------------ */

static uchar usbFunctionSetup(uchar data[8])
{
	usbRequest_t *rq = (void *)data;

	idlePolls = 0; // reset idle polls when we get usb traffic

	static uchar replyBuffer[] = {
		(((uint16_t)BOOTLOADER_ADDRESS) >> 8) & 0xff,
		((uint16_t)BOOTLOADER_ADDRESS) & 0xff,
		SPM_PAGESIZE,
		MICRONUCLEUS_WRITE_SLEEP,
		MICRONUCLEUS_CHIP_ID,
		MICRONUCLEUS_WIRING
	};

	if (rq->bRequest == 0) { // get device info
		usbMsgPtr = replyBuffer;
		return sizeof(replyBuffer);
	} else if (rq->bRequest == 1) { // write page
		//writeLength = rq->wValue.word;
		currentAddress = rq->wIndex.word;

		return USB_NO_MSG;      // hands off work to usbFunctionWrite
	} else if (rq->bRequest == 2) { // erase application
		fireEvent(EVENT_ERASE_APPLICATION);
	} else {                        // exit bootloader
#       if BOOTLOADER_CAN_EXIT
		fireEvent(EVENT_EXECUTE);
#       endif
	}

	return 0;
}

/*
 * Decode an rjmp instruction word and return the absolute address the rjump
 * would jump to.
 *
 * The returned address is in words, offset must be given in words too.
 */
#if VECTOR_SIZE == 2
static inline uint16_t rjmp2addr(const uint16_t rjmp, const uint16_t location)
{
	return rjmp + location + 1;
}
#else
static uint16_t rjmp2addr(const uint16_t rjmp, const uint16_t offset ) {

        // check sign bit
        if ( rjmp & 0x0800 ) {
                // jump backwards
                int16_t addr = rjmp | 0xF000; // extend distance to 16 bit negative int.
                addr += offset + 1;

                return addr & (FLASHEND/2);
        } else {
                int16_t addr = rjmp & 0xFFF;
                return addr + offset + 1;
        }
}
#endif

// read in a page over usb, and write it in to the flash write buffer
static uchar usbFunctionWrite(uchar *data, uchar length)
{
	do {
		// remember vectors or the tinyvector table
		if (currentAddress == RESET_VECTOR_OFFSET * VECTOR_SIZE) {
			vectorTemp[0] = rjmp2addr(*(uint16_t *)data, RESET_VECTOR_OFFSET);
		}
#if VECTOR_SIZE == 4
		else if (currentAddress == RESET_VECTOR_OFFSET * VECTOR_SIZE + 2) {
			if ( ((*(uint16_t *)data) != 0) && ((*(uint16_t *)data) != 0xFFFF) )
				vectorTemp[0] = (*(uint16_t *)data);
		}
#endif
		else if (currentAddress == USB_INTR_VECTOR_NUM * VECTOR_SIZE) {
			vectorTemp[1] = rjmp2addr(((*(uint16_t *)data) + APP_VUSB_OFFSET), USB_INTR_VECTOR_NUM) ;
		}
#if VECTOR_SIZE == 4
		else if (currentAddress == USB_INTR_VECTOR_NUM * VECTOR_SIZE + 2) {
			if ( ((*(uint16_t *)data) != 0) && ((*(uint16_t *)data) != 0xFFFF) )
				vectorTemp[1] = (*(uint16_t *)data);
		}
#endif
		else if (currentAddress >= BOOTLOADER_ADDRESS + TINY_TABLE_LEN) {
			// make sure we don't write over the bootloader!
			break;
		}

		writeWordToPageBuffer(*(uint16_t *)data);
		data += 2; // advance data pointer
		length -= 2;
	} while (length);

	// if we have now reached another page boundary, we're done
	uchar isLast = ((currentAddress % SPM_PAGESIZE) == 0);
	// definitely need this if! seems usbFunctionWrite gets called again in future usbPoll's in the runloop!
	if (isLast) fireEvent(EVENT_WRITE_PAGE);        // ask runloop to write our page

	return isLast;                                  // let vusb know we're done with this request
}

/* ------------------------------------------------------------------------ */

static inline void initForUsbConnectivity(void)
{
	usbInit();
	/* enforce USB re-enumerate: */
	usbDeviceDisconnect(); /* do this while interrupts are disabled */
	_delay_ms(500);
	usbDeviceConnect();
	sei();
}

static inline void tiny85FlashWrites(void)
{
	_delay_us(2000); // TODO: why is this here? - it just adds pointless two level deep loops seems like?
	// write page to flash, interrupts will be disabled for > 4.5ms including erase

	// TODO: Do we need this? Wouldn't the programmer always send full sized pages?
	if (currentAddress % SPM_PAGESIZE)      // when we aren't perfectly aligned to a flash page boundary
		fillFlashWithVectors();         // fill up the rest of the page with 0xFFFF (unprogrammed) bits
	else
		writeFlashPage();               // otherwise just write it
}


// reset system to a normal state and launch user program
static inline void leaveBootloader(void)
{
	_delay_ms(10); // removing delay causes USB errors

	//DBG1(0x01, 0, 0);
	bootLoaderExit();
	cli();
	USB_INTR_ENABLE = 0;
	USB_INTR_CFG = 0;   /* also reset config bits */

	// clear magic word from bottom of stack before jumping to the app
	*(uint8_t *)(RAMEND) = 0x00;
	*(uint8_t *)(RAMEND - 1) = 0x00;

#if !defined(RESTORE_OSCCAL) && !defined(WITH_CRYSTAL)
	// adjust clock to previous calibration value, so user program always starts with same calibration
	// as when it was uploaded originally.
	//
	// Note: not using the stored calibration for the bootloader itself
	//       will allow the device to be un-bricked if somehow an invalid
	//       calibration, that brings the device out-of-spec, ended up in
	//       the tiny table.
	//
	// TODO: Test this and find out, do weneed the +1 offset?
	unsigned char stored_osc_calibration = pgm_read_byte(BOOTLOADER_ADDRESS + STORED_OSCCAL_OFFSET);
	if (stored_osc_calibration != 0xFF && stored_osc_calibration != 0x00) {
		//OSCCAL = stored_osc_calibration; // this should really be a gradual change, but maybe it's alright anyway?
		// do the gradual change - failed to score extra free bytes anyway in 1.06
		while (OSCCAL > stored_osc_calibration) OSCCAL--;
		while (OSCCAL < stored_osc_calibration) OSCCAL++;
	}
#endif

	// jump to application reset vector at end of flash
	__app_reset();
}

int main(void)
{
	/* initialize  */
#ifdef RESTORE_OSCCAL
	uint8_t osccal_default = OSCCAL;
#endif
#if (!SET_CLOCK_PRESCALER) && LOW_POWER_MODE
	uint8_t prescaler_default = CLKPR;
#endif

	wdt_disable();  /* main app may have enabled watchdog */
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
			_delay_us(100);
			idlePolls++;

			// these next two freeze the chip for ~ 4.5ms, breaking usb protocol
			// and usually both of these will activate in the same loop, so host
			// needs to wait > 9ms before next usb request
			if (isEvent(EVENT_ERASE_APPLICATION)) eraseApplication();
			if (isEvent(EVENT_WRITE_PAGE)) tiny85FlashWrites();

#if BOOTLOADER_CAN_EXIT
			if (isEvent(EVENT_EXECUTE)) // when host requests device run uploaded program
			{
				//idlePolls = ((AUTO_EXIT_MS - 21) * 10UL);
				break;
			}

#endif

			clearEvents();
		} while (bootLoaderCondition()); /* main event loop runs so long as bootLoaderCondition remains truthy */
	}

#if LOW_POWER_MODE
	// set clock prescaler to desired clock speed (changing from clkdiv8, or no division, depending on fuses)
#  ifdef SET_CLOCK_PRESCALER
	CLKPR = 1 << CLKPCE;
	CLKPR = SET_CLOCK_PRESCALER;
#  else
	CLKPR = 1 << CLKPCE;
	CLKPR = prescaler_default;
#  endif
#endif

#ifdef RESTORE_OSCCAL
	// slowly bring down OSCCAL to it's original value before launching in to user program
	while (OSCCAL > osccal_default) OSCCAL -= 1;
#endif
	leaveBootloader();
}

/* ------------------------------------------------------------------------ */
