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

#define TINY85_HARDWARE_CONFIG_1 	1
#define TINY85_HARDWARE_CONFIG_2 	2

/* ---------------------------- Hardware Config ---------------------------- */
#define HARDWARE_CONFIG 		TINY85_HARDWARE_CONFIG_2

#define USB_CFG_IOPORTNAME      B
/* This is the port where the USB bus is connected. When you configure it to
 * "B", the registers PORTB, PINB and DDRB will be used.
 */

#ifndef __AVR_ATtiny85__
#	define USB_CFG_DMINUS_BIT      0
/* This is the bit number in USB_CFG_IOPORT where the USB D- line is connected.
 * This may be any bit in the port.
 */
#	define USB_CFG_DPLUS_BIT       2
/* This is the bit number in USB_CFG_IOPORT where the USB D+ line is connected.
 * This may be any bit in the port. Please note that D+ must also be connected
 * to interrupt pin INT0!
 */
#endif
 
#if (defined __AVR_ATtiny85__) && (HARDWARE_CONFIG == TINY85_HARDWARE_CONFIG_1)
#	define USB_CFG_DMINUS_BIT      0
/* This is the bit number in USB_CFG_IOPORT where the USB D- line is connected.
 * This may be any bit in the port.
 */
#	define USB_CFG_DPLUS_BIT       2
/* This is the bit number in USB_CFG_IOPORT where the USB D+ line is connected.
 * This may be any bit in the port, but must be configured as a pin change interrupt.
 */
 #endif
 
#if (defined __AVR_ATtiny85__) && (HARDWARE_CONFIG == TINY85_HARDWARE_CONFIG_2)
#	define USB_CFG_DMINUS_BIT      3
/* This is the bit number in USB_CFG_IOPORT where the USB D- line is connected.
 * This may be any bit in the port.
 */
#	define USB_CFG_DPLUS_BIT       4
/* This is the bit number in USB_CFG_IOPORT where the USB D+ line is connected.
 * This may be any bit in the port, but must be configured as a pin change interrupt.
 */
 #endif

#define USB_CFG_CLOCK_KHZ       (F_CPU/1000)
/* Clock rate of the AVR in MHz. Legal values are 12000, 16000 or 16500.
 * The 16.5 MHz version of the code requires no crystal, it tolerates +/- 1%
 * deviation from the nominal frequency. All other rates require a precision
 * of 2000 ppm and thus a crystal!
 * Default if not specified: 12 MHz
 */

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

//#define HAVE_EEPROM_PAGED_ACCESS    0
/* If HAVE_EEPROM_PAGED_ACCESS is defined to 1, page mode access to EEPROM is
 * compiled in. Whether page mode or byte mode access is used by AVRDUDE
 * depends on the target device. Page mode is only used if the device supports
 * it, e.g. for the ATMega88, 168 etc. You can save quite a bit of memory by
 * disabling page mode EEPROM access. Costs ~ 138 bytes.
 */
//#define HAVE_EEPROM_BYTE_ACCESS     0
/* If HAVE_EEPROM_BYTE_ACCESS is defined to 1, byte mode access to EEPROM is
 * compiled in. Byte mode is only used if the device (as identified by its
 * signature) does not support page mode for EEPROM. It is required for
 * accessing the EEPROM on the ATMega8. Costs ~54 bytes.
 */
#define BOOTLOADER_CAN_EXIT         1
/* If this macro is defined to 1, the boot loader will exit shortly after the
 * programmer closes the connection to the device. Costs ~36 bytes.
 * Required for TINY85MODE
 */
//#define HAVE_CHIP_ERASE             0
/* If this macro is defined to 1, the boot loader implements the Chip Erase
 * ISP command. Otherwise pages are erased on demand before they are written.
 */
//#define SIGNATURE_BYTES             0x1e, 0x93, 0x0b, 0     /* ATtiny85 */
/* This macro defines the signature bytes returned by the emulated USBasp to
 * the programmer software. They should match the actual device at least in
 * memory size and features. If you don't define this, values for ATMega8,
 * ATMega88, ATMega168 and ATMega328 are guessed correctly.
 */

/* The following block guesses feature options so that the resulting code
 * should fit into 2k bytes boot block with the given device and clock rate.
 * Activate by passing "-DUSE_AUTOCONFIG=1" to the compiler.
 * This requires gcc 3.4.6 for small enough code size!
 */
// #if USE_AUTOCONFIG
// #   undef HAVE_EEPROM_PAGED_ACCESS
// #   define HAVE_EEPROM_PAGED_ACCESS     (USB_CFG_CLOCK_KHZ >= 16000)
// #   undef HAVE_EEPROM_BYTE_ACCESS
// #   define HAVE_EEPROM_BYTE_ACCESS      1
// #   undef BOOTLOADER_CAN_EXIT
// #   define BOOTLOADER_CAN_EXIT          1
// #   undef SIGNATURE_BYTES
// #endif /* USE_AUTOCONFIG */

/* ------------------------------------------------------------------------- */

/* Example configuration: Port D bit 3 is connected to a jumper which ties
 * this pin to GND if the boot loader is requested. Initialization allows
 * several clock cycles for the input voltage to stabilize before
 * bootLoaderCondition() samples the value.
 * We use a function for bootLoaderInit() for convenience and a macro for
 * bootLoaderCondition() for efficiency.
 */


#define JUMPER_BIT  0   /* jumper is connected to this bit in port B, active low */

#ifndef MCUCSR          /* compatibility between ATMega8 and ATMega88 */
#   define MCUCSR   MCUSR
#endif

/* tiny85 Architecture Specifics */
#ifndef __AVR_ATtiny85__
#  error "uBoot is only designed for attiny85"
#endif

#define TINY85MODE

// number of bytes before the boot loader vectors to store the tiny application vector table
#define TINYVECTOR_RESET_OFFSET     4
#define TINYVECTOR_USBPLUS_OFFSET   2

#define RESET_VECTOR_OFFSET         0
#define USBPLUS_VECTOR_OFFSET       2

//#if BOOTLOADER_CAN_EXIT == 0
//#    define BOOTLOADER_CAN_EXIT 1
//#endif

// setup interrupt for Pin Change for D+
#define USB_INTR_CFG            PCMSK
#define USB_INTR_CFG_SET        (1 << USB_CFG_DPLUS_BIT)
#define USB_INTR_CFG_CLR        0
#define USB_INTR_ENABLE         GIMSK
#define USB_INTR_ENABLE_BIT     PCIE
#define USB_INTR_PENDING        GIFR
#define USB_INTR_PENDING_BIT    PCIF
#define USB_INTR_VECTOR         PCINT0_vect


/* max 6200ms to not overflow idlePolls variable */
#define AUTO_EXIT_MS    2500
//#define AUTO_EXIT_CONDITION()   (idlePolls > (AUTO_EXIT_MS * 10UL))

// uncomment for chips with clkdiv8 enabled in fuses
//#define LOW_POWER_MODE 1
// restore cpu speed calibration back to 8/16mhz instead of 8.25/16.5mhz
//#define RESTORE_OSCCAL 1
// set clock prescaler to a value before running user program
//#define SET_CLOCK_PRESCALER _BV(CLKPS0) /* divide by 2 for 8mhz */

#define bootLoaderCondition()   (idlePolls < (AUTO_EXIT_MS * 10UL))
#if LOW_POWER_MODE
  // only starts bootloader if USB D- is pulled high on startup - by putting your pullup in to an external connector
  // you can avoid ever entering an out of spec clock speed or waiting on bootloader when that pullup isn't there
  #define bootLoaderStartCondition() \
    (PINB & (_BV(USB_CFG_DMINUS_BIT) | _BV(USB_CFG_DMINUS_BIT))) == _BV(USB_CFG_DMINUS_BIT)
#else
  #define bootLoaderStartCondition() 1
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

#ifndef __ASSEMBLER__   /* assembler cannot parse function definitions */

static inline void  bootLoaderInit(void)
{
#ifndef TINY85MODE
    PORTD |= (1 << JUMPER_BIT);     /* activate pull-up */
    if(!(MCUCSR & (1 << EXTRF)))    /* If this was not an external reset, ignore */
        leaveBootloader();
    MCUCSR = 0;                     /* clear all reset flags for next time */
#endif
}

static inline void  bootLoaderExit(void)
{
#ifndef TINY85MODE
    PORTD = 0;                      /* undo bootLoaderInit() changes */
#endif
}

#endif /* __ASSEMBLER__ */

/* ------------------------------------------------------------------------- */

#endif /* __bootloader_h_included__ */
