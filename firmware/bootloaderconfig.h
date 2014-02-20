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

/*
 *  Bootloader defines
 */
 
#ifndef __ASSEMBLER__
  typedef union {
    uint16_t w;
    uint8_t b[2];
  } uint16_union_t;
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

#define TINY85_HARDWARE_CONFIG_1  1
#define TINY85_HARDWARE_CONFIG_2  2

/* ---------------------------- Hardware Config ---------------------------- */
#define HARDWARE_CONFIG     TINY85_HARDWARE_CONFIG_2

#define USB_CFG_IOPORTNAME      B
  /* This is the port where the USB bus is connected. When you configure it to
   * "B", the registers PORTB, PINB and DDRB will be used.
   */

#ifndef __AVR_ATtiny85__
  # define USB_CFG_DMINUS_BIT      0
  /* This is the bit number in USB_CFG_IOPORT where the USB D- line is connected.
   * This may be any bit in the port.
   */
  #define USB_CFG_DPLUS_BIT       2
  /* This is the bit number in USB_CFG_IOPORT where the USB D+ line is connected.
   * This may be any bit in the port. Please note that D+ must also be connected
   * to interrupt pin INT0!
   */
#endif
 
#if (defined __AVR_ATtiny85__) && (HARDWARE_CONFIG == TINY85_HARDWARE_CONFIG_1)
  #define USB_CFG_DMINUS_BIT      0
  /* This is the bit number in USB_CFG_IOPORT where the USB D- line is connected.
   * This may be any bit in the port.
   */
  #define USB_CFG_DPLUS_BIT       2
  /* This is the bit number in USB_CFG_IOPORT where the USB D+ line is connected.
   * This may be any bit in the port, but must be configured as a pin change interrupt.
   */
#endif
 
#if (defined __AVR_ATtiny85__) && (HARDWARE_CONFIG == TINY85_HARDWARE_CONFIG_2)
#define USB_CFG_DMINUS_BIT      3
/* This is the bit number in USB_CFG_IOPORT where the USB D- line is connected.
 * This may be any bit in the port.
 */
#define USB_CFG_DPLUS_BIT       4
/* This is the bit number in USB_CFG_IOPORT where the USB D+ line is connected.
 * This may be any bit in the port, but must be configured as a pin change interrupt.
 */
#endif

#define USB_CFG_CLOCK_KHZ       (F_CPU/1000)
/* Clock rate of the AVR in kHz. Legal values are 12000, 16000 or 16500.
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
 
/* ----------------------- Optional MCU Description ------------------------ */

/* tiny85 Architecture Specifics */
#ifndef __AVR_ATtiny85__
#  error "uBoot is only designed for attiny85"
#endif
#define TINY85MODE

/* ------------- Set up interrupt configuration (CPU specific) -------------- */

// setup interrupt for Pin Change for D+
#define USB_INTR_CFG            PCMSK
#define USB_INTR_CFG_SET        (1 << USB_CFG_DPLUS_BIT)
#define USB_INTR_CFG_CLR        0
#define USB_INTR_ENABLE         GIMSK
#define USB_INTR_ENABLE_BIT     PCIE
#define USB_INTR_PENDING        GIFR
#define USB_INTR_PENDING_BIT    PCIF
#define USB_INTR_VECTOR         PCINT0_vect

// Microcontroller vectortable entries in the flash
#define RESET_VECTOR_OFFSET         0

// number of bytes before the boot loader vectors to store the tiny application vector table
#define TINYVECTOR_RESET_OFFSET     4
#define TINYVECTOR_OSCCAL_OFFSET    6

/* ------------------------------------------------------------------------ */
// postscript are the few bytes at the end of programmable memory which store tinyVectors
#define POSTSCRIPT_SIZE 6
#define PROGMEM_SIZE (BOOTLOADER_ADDRESS - POSTSCRIPT_SIZE) /* max size of user program */

/* ------------------------------------------------------------------------- */

/*
 *  Define Bootloader entry condition
 * 
 *  If the entry condition is not met, the bootloader will not be activated and the user program
 *  is executed directly after a reset. If no user program has been loaded, the bootloader
 *  is always active.
 * 
 *  ENTRY_ALWAYS        Always activate the bootloader after reset. Requires the least
 *                      amount of code.
 *
 *  ENTRY_WATCHDOG      Activate the bootloader after a watchdog reset. This can be used
 *                      to enter the bootloader from the user program.
 *                      Adds 22 bytes.
 *
 *  ENTRY_EXT_RESET     Activate the bootloader after an external reset was issued by 
 *                      pulling the reset pin low. It may be necessary to add an external
 *                      pull-up resistor to the reset pin if this entry method appears to
 *                      behave unreliably.
 *                      Adds 22 bytes.
 *
 *  ENTRY_JUMPER        Activate the bootloader when a specific pin is pulled low by an 
 *                      external jumper. 
 *                      Adds 34 bytes.
 *
 *       JUMPER_PIN     Pin the jumper is connected to. (e.g. PB0)
 *       JUMPER_PORT    Port out register for the jumper (e.g. PORTB)  
 *       JUMPER_DDR     Port data direction register for the jumper (e.g. DDRB)  
 *       JUMPER_INP     Port inout register for the jumper (e.g. PINB)  
 * 
 */

#define ENTRYMODE ENTRY_ALWAYS

#define JUMPER_PIN    PB0
#define JUMPER_PORT   PORTB 
#define JUMPER_DDR    DDRB 
#define JUMPER_INP    PINB 
 
#define ENTRY_ALWAYS    1
#define ENTRY_WATCHDOG  2
#define ENTRY_EXT_RESET 3
#define ENTRY_JUMPER    4

#if ENTRYMODE==ENTRY_ALWAYS
  #define bootLoaderInit()
  #define bootLoaderExit()
  #define bootLoaderStartCondition() 1
#elif ENTRYMODE==ENTRY_WATCHDOG
  #define bootLoaderInit()
  #define bootLoaderExit()
  #define bootLoaderStartCondition() (MCUSR&_BV(WDRF))
#elif ENTRYMODE==ENTRY_EXT_RESET
  #define bootLoaderInit()
  #define bootLoaderExit()
  #define bootLoaderStartCondition() (MCUSR&_BV(EXTRF))
#elif ENTRYMODE==ENTRY_JUMPER
  // Enable pull up on jumper pin and delay to stabilize input    
  #define bootLoaderInit()   {JUMPER_DDR&=~_BV(JUMPER_PIN);JUMPER_PORT|=_BV(JUMPER_PIN);_delay_ms(1);}
  #define bootLoaderExit()   {JUMPER_PORT&=~_BV(JUMPER_PIN);}
  #define bootLoaderStartCondition() (!(JUMPER_INP&_BV(JUMPER_PIN)))
#else
   #error "No entry mode defined"
#endif

/*
 * Define bootloader timeout value. 
 * 
 *  The bootloader will only time out if a user program was loaded.
 * 
 *  AUTO_EXIT_NO_USB_MS        The bootloader will exit after this delay if no USB is connected.
 *                             Set to 0 to disable
 *                             Adds ~6 bytes.
 *                             (This will wait for an USB SE0 reset from the host)
 *
 *  AUTO_EXIT_MS               The bootloader will exit after this delay if no USB communication
 *                             from the host tool was received.
 *                             Set to 0 to disable
 *  
 *  All values are approx. in milliseconds
 */

#define AUTO_EXIT_NO_USB_MS    0
#define AUTO_EXIT_MS           5000

 /*
 *	Defines the setting of the RC-oscillator calibration after quitting the bootloader. (OSCCAL)
 * 
 *  OSCCAL_RESTORE            Set this to '1' to revert to factory calibration, which is 16.0 MHZ +/-10%
 *                            Adds ~14 bytes.
 *
 *  OSCCAL_16.5MHz            Set this to '1' to use the same calibration as during program upload.
 *                            This value is 16.5Mhz +/-1% as calibrated from the USB timing. Please note
 *                            that only true if the ambient temperature does not change.
 *                            This is the default behaviour of the Digispark.
 *                            Adds ~38 bytes.
 *
 *  OSCCAL_HAVE_XTAL          Set this to '1' if you have an external crystal oscillator. In this case no attempt
 *                            will be made to calibrate the oscillator. You should deactivate both options above
 *                            if you use this to avoid redundant code.
 *
 *  If both options are selected, OSCCAL_RESTORE takes precedence.
 *
 *  If no option is selected, OSCCAL will be left untouched and stay at either 16.0Mhz or 16.5Mhz depending
 *  on whether the bootloader was activated. This will take the least memory. You can use this if your program
 *  comes with its own OSCCAL calibration or an external clock source is used. 
 */
 
#define OSCCAL_RESTORE 1
#define OSCCAL_16_5MHz 0
#define OSCCAL_HAVE_XTAL 0
  
/*  
 *  Defines handling of an indicator LED while the bootloader is active.  
 * 
 *  LED_PRESENT               Set this this to '1' to active all LED related code. If this is 0, all other
 *                            defines are ignored.
 *                            Adds 18 bytes depending on implementation.								
 *
 *  LED_DDR,LED_PORT,LED_PIN  Where is your LED connected?
 *
 *  LED_INIT                  Called once after bootloader entry
 *  LED_EXIT                  Called once during bootloader exit
 *  LED_MACRO                 Called in the main loop with the idle counter as parameter.
 *                            Use to define pattern.
 */ 

#define	LED_PRESENT	    1

#define	LED_DDR			DDRB
#define LED_PORT		PORTB
#define	LED_PIN			PB1

#if LED_PRESENT
//  #define LED_INIT(x)		LED_PORT &=~_BV(LED_PIN);   // Use this with low active LED
  #define LED_INIT(x)		LED_PORT  = _BV(LED_PIN);   // Use this with high active LED
  #define LED_EXIT(x)		LED_DDR  &=~_BV(LED_PIN);
  #define LED_MACRO(x)	if ( x & 0xd0 ) {LED_DDR&=~_BV(LED_PIN);} else {LED_DDR|=_BV(LED_PIN);}
#else
  #define LED_INIT(x)		
  #define LED_EXIT(x)		
  #define LED_MACRO(x)	
#endif
/* ------------------------------------------------------------------------- */

#endif /* __bootloader_h_included__ */
