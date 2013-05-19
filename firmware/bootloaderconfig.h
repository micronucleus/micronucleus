/* Name: bootloaderconfig.h
 * Project: USBaspLoader
 * Author: Christian Starkjohann
 * Creation Date: 2007-12-08
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * Portions Copyright: (c) 2012 Louis Beaudoin
 * License: GNU GPL v2 (see License.txt)
 * This Revision: $Id: bootloaderconfig.h 729 2009-03-20 09:03:58Z cs $
 */

#ifndef __bootloaderconfig_h_included__
#define __bootloaderconfig_h_included__

// uncomment this to enable the 'jumper from d5 to gnd to enable programming' mode
//#define BUILD_JUMPER_MODE 1

#ifndef BOOTLOADER_ADDRESS
#define BOOTLOADER_ADDRESS 0
#endif

/*
General Description:
This file (together with some settings in Makefile) configures the boot loader
according to the hardware.

This file contains (besides the hardware configuration normally found in
usbconfig.h) two functions or macros: bootLoaderInit() and
bootLoaderCondition(). Whether you implement them as macros or as static
inline functions is up to you, decide based on code size and convenience.

bootLoaderInit() is called as one of the first actions after reset. It should
be a minimum initialization of the hardware so that the boot loader condition
can be read. This will usually consist of activating a pull-up resistor for an
external jumper which selects boot loader mode.

bootLoaderCondition() is called immediately after initialization and in each
main loop iteration. If it returns TRUE, the boot loader will be active. If it
returns FALSE, the boot loader jumps to address 0 (the loaded application)
immediately.

For compatibility with Thomas Fischl's avrusbboot, we also support the macro
names BOOTLOADER_INIT and BOOTLOADER_CONDITION for this functionality. If
these macros are defined, the boot loader uses them.
*/

#define USB_CFG_CLOCK_KHZ       (F_CPU/1000)
/* Clock rate of the AVR in MHz. Legal values are 12000, 16000 or 16500.
 * The 16.5 MHz version of the code requires no crystal, it tolerates +/- 1%
 * deviation from the nominal frequency. All other rates require a precision
 * of 2000 ppm and thus a crystal!
 * Default if not specified: 12 MHz
 */

#define RESET_VECTOR_OFFSET         0

/* ----------------------- Optional Hardware Config ------------------------ */

/* #define USB_CFG_PULLUP_IOPORTNAME   D */
/* If you connect the 1.5k pullup resistor from D- to a port pin instead of
 * V+, you can connect and disconnect the device from firmware by calling
 * the macros usbDeviceConnect() and usbDeviceDisconnect() (see usbdrv.h).
 * This constant defines the port on which the pullup resistor is connected.
 */
/* #define USB_CFG_PULLUP_BIT          4 */
/* This constant defines the bit number in USB_CFG_PULLUP_IOPORT (defined
 * above) where the 1.5k pullup resistor is connected. See description
 * above for details.
 */

/* ------------------------------------------------------------------------- */
/* ---------------------- feature / code size options ---------------------- */
/* ------------------------------------------------------------------------- */

#define BOOTLOADER_CAN_EXIT         1
/* If this macro is defined to 1, the boot loader will exit shortly after the
 * programmer closes the connection to the device. Costs ~36 bytes.
 * Required for TINY85MODE
 */

/* ------------------------------------------------------------------------- */

/* Example configuration: Port D bit 3 is connected to a jumper which ties
 * this pin to GND if the boot loader is requested. Initialization allows
 * several clock cycles for the input voltage to stabilize before
 * bootLoaderCondition() samples the value.
 * We use a function for bootLoaderInit() for convenience and a macro for
 * bootLoaderCondition() for efficiency.
 */


#define JUMPER_BIT  0   /* jumper is connected to this bit in port B, active low */

/* max 6200ms to not overflow idlePolls variable */
#define AUTO_EXIT_MS    5000
//#define AUTO_EXIT_CONDITION()   (idlePolls > (AUTO_EXIT_MS * 10UL))

// uncomment for chips with clkdiv8 enabled in fuses
//#define LOW_POWER_MODE 1
// restore cpu speed calibration back to 8/16mhz instead of 8.25/16.5mhz
//#define RESTORE_OSCCAL 1
// set clock prescaler to a value before running user program
//#define SET_CLOCK_PRESCALER _BV(CLKPS0) /* divide by 2 for 8mhz */

#ifdef BUILD_JUMPER_MODE
	#define START_JUMPER_PIN 5
	#define digitalRead(pin) (PINB & _BV(pin))
	#define bootLoaderStartCondition() (!digitalRead(START_JUMPER_PIN))
	#define bootLoaderCondition() 1
	
	#ifndef __ASSEMBLER__   /* assembler cannot parse function definitions */
		static inline void  bootLoaderInit(void) {
		 	// DeuxVis pin-5 pullup
			DDRB |= _BV(START_JUMPER_PIN); // is an input
			PORTB |= _BV(START_JUMPER_PIN); // has pullup enabled
			_delay_ms(10);
		}
		static inline void  bootLoaderExit(void) {
		  // DeuxVis pin-5 pullup
		 	PORTB = 0;
		  DDRB = 0;
		}
	#endif /* __ASSEMBLER__ */
#else
	#define bootLoaderInit()
	#define bootLoaderExit()
	#define bootLoaderCondition()   (idlePolls < (AUTO_EXIT_MS * 10UL))
	#if LOW_POWER_MODE
	  // only starts bootloader if USB D- is pulled high on startup - by putting your pullup in to an external connector
	  // you can avoid ever entering an out of spec clock speed or waiting on bootloader when that pullup isn't there
	  #define bootLoaderStartCondition() \
	    (PINB & (_BV(USB_CFG_DMINUS_BIT) | _BV(USB_CFG_DMINUS_BIT))) == _BV(USB_CFG_DMINUS_BIT)
	#else
	  #define bootLoaderStartCondition() 1
	#endif
#endif

/* ----------------------- Optional MCU Description ------------------------ */

/* The following configurations have working defaults in usbdrv.h. You
 * usually don't need to set them explicitly. Only if you want to run
 * the driver on a device which is not yet supported or with a compiler
 * which is not fully supported (such as IAR C) or if you use a different
 * interrupt than INT0, you may have to define some of these.
 */
/* #define USB_INTR_CFG            MCUCR */
/* #define USB_INTR_CFG_SET        ((1 << ISC00) | (1 << ISC01)) */
/* #define USB_INTR_CFG_CLR        0 */
/* #define USB_INTR_ENABLE         GIMSK */
/* #define USB_INTR_ENABLE_BIT     INT0 */
/* #define USB_INTR_PENDING        GIFR */
/* #define USB_INTR_PENDING_BIT    INTF0 */
/* #define USB_INTR_VECTOR         INT0_vect */

// todo: change to pin 5
//#define DEUXVIS_JUMPER_PIN 5
//#define digitalRead(pin) ((PINB >> pin) & 0b00000001)
//#define bootLoaderStartCondition() (!digitalRead(DEUXVIS_JUMPER_PIN))
//#define bootLoaderCondition()   (1)

#ifndef __ASSEMBLER__   /* assembler cannot parse function definitions */

//static inline void  bootLoaderInit(void) {
//  // DeuxVis pin-5 pullup
//  DDRB |= _BV(DEUXVIS_JUMPER_PIN); // is an input
//  PORTB |= _BV(DEUXVIS_JUMPER_PIN); // has pullup enabled
//  _delay_ms(10);
//}
//static inline void  bootLoaderExit(void) {
  // DeuxVis pin-5 pullup
//  PORTB = 0;
//  DDRB = 0;
//}

#endif /* __ASSEMBLER__ */

/* ------------------------------------------------------------------------- */

#endif /* __bootloader_h_included__ */
