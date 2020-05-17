/* Name: bootloaderconfig.h
 * Micronucleus configuration file.
 * This file (together with some settings in Makefile.inc) configures the boot loader
 * according to the hardware.
 *
 * Controller type: ATtiny 85 - 16.5 MHz
 * Configuration:   Default configuration + ENTRY_POWER_ON + START_WITHOUT_PULLUP + FAST_EXIT_NO_USB_MS=300
 *       USB D- :   PB3
 *       USB D+ :   PB4
 *       Entry  :   Always
 *       LED    :   None
 *       OSCCAL :   Stays at 16 MHz
 * Note: Uses 16.5 MHz V-USB implementation with PLL
 * Last Change:     Mar 16,2014
 *
 * License: GNU GPL v2 (see License.txt
 */
#ifndef __bootloaderconfig_h_included__
#define __bootloaderconfig_h_included__

/* ------------------------------------------------------------------------- */
/*                       Hardware configuration.                             */
/*      Change this according to your CPU and USB configuration              */
/* ------------------------------------------------------------------------- */

#define USB_CFG_IOPORTNAME      B
  /* This is the port where the USB bus is connected. When you configure it to
   * "B", the registers PORTB, PINB and DDRB will be used.
   */

#define USB_CFG_DMINUS_BIT      3
/* This is the bit number in USB_CFG_IOPORT where the USB D- line is connected.
 * This may be any bit in the port.
 */
#define USB_CFG_DPLUS_BIT       4
/* This is the bit number in USB_CFG_IOPORT where the USB D+ line is connected.
 * This may be any bit in the port, but must be configured as a pin change interrupt.
 */

#define USB_CFG_CLOCK_KHZ       (F_CPU/1000)
/* Clock rate of the AVR in kHz. Legal values are 12000, 12800, 15000, 16000,
 * 16500, 18000 and 20000. The 12.8 MHz and 16.5 MHz versions of the code
 * require no crystal, they tolerate +/- 1% deviation from the nominal
 * frequency. All other rates require a precision of 2000 ppm and thus a
 * crystal!
 * Since F_CPU should be defined to your actual clock rate anyway, you should
 * not need to modify this setting.
 */

/* ----------------------- Optional Hardware Config ------------------------ */

//#define USB_CFG_PULLUP_IOPORTNAME   B
/* If you connect the 1.5k pullup resistor from D- to a port pin instead of
 * V+, you can connect and disconnect the device from firmware by calling
 * the macros usbDeviceConnect() and usbDeviceDisconnect() (see usbdrv.h).
 * This constant defines the port on which the pullup resistor is connected.
 */
//#define USB_CFG_PULLUP_BIT          0
/* This constant defines the bit number in USB_CFG_PULLUP_IOPORT (defined
 * above) where the 1.5k pullup resistor is connected. See description
 * above for details.
 */

/* ------------- Set up interrupt configuration (CPU specific) --------------   */
/* The register names change quite a bit in the ATtiny family. Pay attention    */
/* to the manual. Note that the interrupt flag system is still used even though */
/* interrupts are disabled. So this has to be configured correctly.             */

/* ------------- Set up interrupt configuration (CPU specific) --------------   */
/* The register names change quite a bit in the ATtiny family. Pay attention    */
/* to the manual. Note that the interrupt flag system is still used even though */
/* interrupts are disabled. So this has to be configured correctly.             */


// setup interrupt for Pin Change for D+
#define USB_INTR_CFG            PCMSK // Pin interrupt enable register
#define USB_INTR_CFG_SET        (1 << USB_CFG_DPLUS_BIT) // mask for pin in pin interrupt enable register
#define USB_INTR_CFG_CLR        0
#define USB_INTR_ENABLE         GIMSK // Global interrupt enable register
#define USB_INTR_ENABLE_BIT     PCIE  // Bit position in global interrupt enable register
#define USB_INTR_PENDING        GIFR  // Register to read interrupt flag
#define USB_INTR_PENDING_BIT    PCIF  // Bit position in register to read interrupt flag

/* ------------------------------------------------------------------------- */
/*       Configuration relevant to the CPU the bootloader is running on      */
/* ------------------------------------------------------------------------- */

// how many milliseconds should host wait till it sends another erase or write?
// needs to be above 4.5 (and a whole integer) as avr freezes maximum for 4.5ms
// while writing a FLASH page (even for 128 byte page size:-))
#define MICRONUCLEUS_WRITE_SLEEP 5


/* ---------------------- feature / code size options ---------------------- */
/*               Configure the behavior of the bootloader here               */
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
 *  ENTRY_POWER_ON      Activate the bootloader after power on. This is what you need
 *                      for normal development with Digispark boards.
 *                      !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *                      Since the reset flags are no longer cleared by micronucleus
 *                      you must clear them with "MCUSR = 0;" in your setup() routine
 *                      after saving or evaluating them to make this mode work.
 *                      If you do not reset the flags, the bootloader will be entered even
 *                      after reset, since the "power on reset flag" PORF in MCUSR is still set.
 *                      Adds 18 bytes.
 *
 *  ENTRY_WATCHDOG      Activate the bootloader after a watchdog reset. This can be used
 *                      to enter the bootloader from the user program.
 *                      Adds 22 bytes.
 *
 *  ENTRY_EXT_RESET     Activate the bootloader after an external reset was issued by
 *                      pulling the reset pin low. It may be necessary to add an external
 *                      pull-up resistor to the reset pin if this entry method appears to
 *                      behave unreliably.
 *                      Adds 24 bytes.
 *
 *  ENTRY_JUMPER        Activate the bootloader when a specific pin is pulled low by an
 *                      external jumper.
 *                      Adds 34 bytes.
 *
 *       JUMPER_PIN     Pin the jumper is connected to. (e.g. PB0)
 *       JUMPER_PORT    Port out register for the jumper (e.g. PORTB)
 *       JUMPER_DDR     Port data direction register for the jumper (e.g. DDRB)
 *       JUMPER_INP     Port input register for the jumper (e.g. PINB)
 *
 */

// Internal implementation, don't change this unless you want to add an entry mode.
#define ENTRY_ALWAYS    1
#define ENTRY_WATCHDOG  2
#define ENTRY_EXT_RESET 3
#define ENTRY_JUMPER    4
#define ENTRY_POWER_ON  5

#define ENTRYMODE ENTRY_POWER_ON

#define JUMPER_PIN    PB0
#define JUMPER_PORT   PORTB
#define JUMPER_DDR    DDRB
#define JUMPER_INP    PINB

#if ENTRYMODE==ENTRY_ALWAYS
  #define bootLoaderInit()
  #define bootLoaderExit()
  #define bootLoaderStartCondition() 1
#elif ENTRYMODE==ENTRY_WATCHDOG
  #define bootLoaderInit()
  #define bootLoaderExit()
  #define bootLoaderStartCondition() (MCUSR & _BV(WDRF))
#elif ENTRYMODE==ENTRY_EXT_RESET
// On my ATtiny85 I have always 0x03 EXTRF | PORF after power on.
// After reset only EXTRF is NEWLY set.
// So we must reset at least PORF flag ALWAYS after checking for entry condition,
// otherwise entry condition will NEVER be true if application does not reset PORF.
// To be able to interpret MCUSR flags in user program, it is copied to the OCR1C register.
// In turn we can just clear MCUSR, which saves flash.
  #define bootLoaderInit()
  #define bootLoaderExit() {OCR1C=MCUSR; MCUSR = 0;} // Adds 6 bytes
  #define bootLoaderStartCondition() (MCUSR == _BV(EXTRF)) // Adds 18 bytes
#elif ENTRYMODE==ENTRY_JUMPER
  // Enable pull up on jumper pin and delay to stabilize input
  #define bootLoaderInit()   {JUMPER_DDR &= ~_BV(JUMPER_PIN); JUMPER_PORT |= _BV(JUMPER_PIN); _delay_ms(1);}
  #define bootLoaderExit()   {JUMPER_PORT &= ~_BV(JUMPER_PIN);}
  #define bootLoaderStartCondition() (!(JUMPER_INP & _BV(JUMPER_PIN)))
#elif ENTRYMODE==ENTRY_POWER_ON
  #define bootLoaderInit()
  #define bootLoaderExit()
  #define bootLoaderStartCondition() (MCUSR&_BV(PORF))
#else
   #error "No entry mode defined"
#endif

/*
 * Define bootloader timeout value.
 *
 *  The bootloader will only time out if a user program was loaded.
 *
 *  FAST_EXIT_NO_USB_MS        The bootloader will exit after this delay if no USB is connected after the initial 300 ms disconnect and connect.
 *                             Set to < 120 to disable.
 *                             Adds 8 bytes.
 *                             (This will wait for FAST_EXIT_NO_USB_MS milliseconds for an USB SE0 reset from the host, otherwise exit)
 *
 *  AUTO_EXIT_MS               The bootloader will exit after this delay if no USB communication from the host tool was received.
 *                             Set to 0 to disable
 *
 *  All values are approx. in milliseconds
 */

// I observed 2 resets. First is 100 ms after initial connecting to USB lasting 65 ms and the second 90 ms later and also 65 ms.
// On my old HP laptop I have different timing: First reset is 220 ms after initial connecting to USB lasting 300 ms and the second is missing.
#define FAST_EXIT_NO_USB_MS     300 // Values below 120 are ignored. Effective timeout is 300 + FAST_EXIT_NO_USB_MS.
#define AUTO_EXIT_MS           6000

/* ----------------------- Optional Timeout Config ------------------------ */

//#define START_WITHOUT_PULLUP
/* If you do not have the 1.5k pullup resistor connected directly from D- to ATtiny VCC
 * to save power for battery operated applications, you must insert a diode
 * between USB V+ and ATiny VCC and connect the resistor directly to USB V+.
 * If not connected to USB we then have an endless USB reset condition,
 * which must be handled separately by commenting out the define.
 * This handling adds 14 to 16 bytes to the code size.
 * This handling also works well with standard / unmodified pullup connection :-).
 */

 /*
 *  Defines the setting of the RC-oscillator calibration after quitting the bootloader. (OSCCAL)
 *
 *  OSCCAL_RESTORE_DEFAULT    Set this to '1' to revert to OSCCAL factory calibration after bootloader exit.
 *                            This is 8 MHz +/-2% on most devices or 16 MHz on the ATtiny 85 with activated PLL.
 *                            Adds ~14 bytes.
 *
 *  OSCCAL_SAVE_CALIB         Set this to '1' to save the OSCCAL calibration during program upload.
 *                            This value will be reloaded after reset and will also be used for the user
 *                            program unless "OSCCAL_RESTORE_DEFAULT" is active. This allows calibrate the internal
 *                            RC oscillator to the F_CPU target frequency +/-1% from the USB timing. Please note
 *                            that this is only true if the ambient temperature does not change.
 *                            Adds ~38 bytes.
 *
 *  OSCCAL_HAVE_XTAL          Set this to '1' if you have an external crystal oscillator. In this case no attempt
 *                            will be made to calibrate the oscillator. You should deactivate both options above
 *                            if you use this to avoid redundant code.
 *
 *  If both options are selected, OSCCAL_RESTORE_DEFAULT takes precedence.
 *
 *  If no option is selected, OSCCAL will be left untouched and stays at either factory calibration or F_CPU depending
 *  on whether the bootloader was activated. This will take the least memory. You can use this if your program
 *  comes with its own OSCCAL calibration or an external clock source is used.
 */

#define OSCCAL_RESTORE_DEFAULT 0
#define OSCCAL_SAVE_CALIB 1
#define OSCCAL_HAVE_XTAL 0

/*
 *  Defines handling of an indicator LED while the bootloader is active.
 *
 *  LED_MODE                  Define behavior of attached LED or suppress LED code.
 *
 *          NONE              Do not generate LED code (gains 18 bytes).
 *          ACTIVE_HIGH       LED is on when output pin is high. This will toggle between 1 and 0.
 *          ACTIVE_LOW        LED is on when output pin is low.  This will toggle between Z and 0. + 2 bytes
 *
 *  LED_DDR,LED_PORT,LED_PIN  Where is your LED connected?
 *
 */

#define NONE        0
#define ACTIVE_HIGH 1
#define ACTIVE_LOW  2

#define LED_MODE    ACTIVE_HIGH

#define LED_DDR     DDRB
#define LED_PORT    PORTB
#define LED_PIN     PB1

/*
 *  This is the implementation of the LED code. Change the configuration above unless you want to
 *  change the led behavior
 *
 *  LED_INIT                  Called once after bootloader entry
 *  LED_EXIT                  Called once during bootloader exit
 *  LED_MACRO                 Called in the main loop with the idle counter as parameter.
 *                            Use to define pattern.
 */

#if LED_MODE==ACTIVE_HIGH
  #define LED_INIT(x)   LED_DDR |= _BV(LED_PIN);
  #define LED_EXIT(x)   LED_PORT &= ~_BV(LED_PIN);
  #define LED_MACRO(x)  if ( x & 0x4c ) {LED_PORT &= ~_BV(LED_PIN);} else {LED_PORT |= _BV(LED_PIN);}
#elif LED_MODE==ACTIVE_LOW
  #define LED_INIT(x)   LED_PORT &= ~_BV(LED_PIN);
  #define LED_EXIT(x)   LED_DDR &= ~_BV(LED_PIN);
  #define LED_MACRO(x)  if ( x & 0x4c ) {LED_DDR &= ~_BV(LED_PIN);} else {LED_DDR |= _BV(LED_PIN);}
#elif LED_MODE==NONE
  #define LED_INIT(x)
  #define LED_EXIT(x)
  #define LED_MACRO(x)
#endif

#endif /* __bootloader_h_included__ */
