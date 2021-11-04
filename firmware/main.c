/*
 * Project: Micronucleus -  v2.5
 *
 * Micronucleus V2.5             (c) 2020 Armin Joachimsmeyer armin.joachimsmeyer@gmail.com
 * Micronucleus V2.04            (c) 2016 Tim Bo"scke - cpldcpu@gmail.com
 * Micronucleus V2.3             (c) 2016 Tim Bo"scke - cpldcpu@gmail.com
 *                               (c) 2014 Shay Green
 * Original Micronucleus         (c) 2012 Jenna Fox
 *
 * Based on USBaspLoader-tiny85  (c) 2012 Louis Beaudoin
 * Based on USBaspLoader         (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 *
 *  This file is part of micronucleus https://github.com/micronucleus/micronucleus.
 *
 *  Micronucleus is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

#define MICRONUCLEUS_VERSION_MAJOR 2
#define MICRONUCLEUS_VERSION_MINOR 6

#define RECONNECT_DELAY_MILLIS 300 // Time between disconnect and connect. Even 250 is to fast!
#define __DELAY_BACKWARD_COMPATIBLE__ // Saves 2 bytes at _delay_ms(). Must be declared before the include util/delay.h

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <util/delay.h>

#include "bootloaderconfig.h"
#include "usbdrv/usbdrv.c"

// Microcontroller vector table entries in the flash
#define RESET_VECTOR_OFFSET         0 // is 0 for all ATtinies

// number of bytes before the boot loader vectors to store the application reset vector
#define TINYVECTOR_RESET_OFFSET     4 // need 4 bytes for user reset vector to support devices with more than 8k FLASH
#if OSCCAL_SAVE_CALIB
#define TINYVECTOR_OSCCAL_OFFSET    6
#endif

// Postscript are the few bytes at the end of programmable memory which store user program reset vector and optionally OSCCAL calibration
#ifndef POSTSCRIPT_SIZE
#  if OSCCAL_SAVE_CALIB
#define POSTSCRIPT_SIZE TINYVECTOR_OSCCAL_OFFSET
#  else
#define POSTSCRIPT_SIZE TINYVECTOR_RESET_OFFSET
#  endif
#endif
#define PROGMEM_SIZE (BOOTLOADER_ADDRESS - POSTSCRIPT_SIZE) /* max size of user program */

// verify the bootloader address aligns with page size
#if (defined __AVR_ATtiny841__)||(defined __AVR_ATtiny441__)||(defined __AVR_ATtiny1634__)
#if BOOTLOADER_ADDRESS % ( SPM_PAGESIZE * 4 ) != 0
#error "BOOTLOADER_ADDRESS in makefile must be a multiple of chip's pagesize"
#endif
#else
#if BOOTLOADER_ADDRESS % SPM_PAGESIZE != 0
#error "BOOTLOADER_ADDRESS in makefile must be a multiple of chip's pagesize"
#endif
#endif

#if SPM_PAGESIZE>256
#error "Micronucleus only supports pagesizes up to 256 bytes"
#endif

#if ((AUTO_EXIT_MS>0) && (AUTO_EXIT_MS<1000))
#error "Do not set AUTO_EXIT_MS to below 1s to allow Micronucleus to function properly"
#endif

#if ((FAST_EXIT_NO_USB_MS>0) && (FAST_EXIT_NO_USB_MS<120))
#warning "Values below 120 ms are not possible for FAST_EXIT_NO_USB_MS"
#endif

// Device configuration reply
// Length: 6 bytes
//   Byte 0:  User program memory size, high byte
//   Byte 1:  User program memory size, low byte
//   Byte 2:  Flash Pagesize in bytes
//   Byte 3:  Page write timing in ms.
//    Bit 7 '0': Page erase time equals page write time
//    Bit 7 '1': Page erase time equals page write time divided by 4
//   Byte 4:  SIGNATURE_1
//   Byte 5:  SIGNATURE_2
//   Byte 6:  Bootloader feature flags
//   Byte 7:  Application major version<< 4 | Application minor version, 0 = no entry
#if FAST_EXIT_NO_USB_MS > 120
#define FAST_EXIT_FEATURE_FLAG 0x10
#else
#define FAST_EXIT_FEATURE_FLAG 0x00
#endif
#if defined(SAVE_MCUSR)
#define SAVE_MCUSR_FEATURE_FLAG 0x20
#else
#define SAVE_MCUSR_FEATURE_FLAG 0x00
#endif

#if defined(STORE_CONFIGURATION_REPLY_IN_RAM)
const uint8_t configurationReply[8] = { // dummy comment for eclipse formatter
    (((uint16_t) PROGMEM_SIZE) >> 8) & 0xff,//
    ((uint16_t) PROGMEM_SIZE) & 0xff,
    SPM_PAGESIZE,
    MICRONUCLEUS_WRITE_SLEEP,
    SIGNATURE_1,
    SIGNATURE_2,
    FAST_EXIT_FEATURE_FLAG | ENTRYMODE, //
    0x0// 0x15 -> Application version 1.5
};
#else
PROGMEM const uint8_t configurationReply[8] = { // dummy comment for eclipse formatter
        (((uint16_t) PROGMEM_SIZE) >> 8) & 0xff, //
        ((uint16_t) PROGMEM_SIZE) & 0xff,
        SPM_PAGESIZE,
        MICRONUCLEUS_WRITE_SLEEP,
        SIGNATURE_1,
        SIGNATURE_2,
        FAST_EXIT_FEATURE_FLAG | ENTRYMODE, //
                0x0 // 0x15 -> Application version 1.5
        };
#endif

typedef union {
    uint16_t w;
    uint8_t b[2];
} uint16_union_t;

#if OSCCAL_RESTORE_DEFAULT
  register uint8_t      osccal_default  asm("r2");
#endif

register uint16_union_t currentAddress asm("r4");  // r4/r5 current progmem address, used for erasing and writing
register uint16_union_t idlePolls asm("r6");  // r6/r7 idle counter - each tick is 5 milliseconds

// command used to trigger functions to run in the main loop
enum {
    cmd_local_nop = 0,
    cmd_device_info = 0,
    cmd_transfer_page = 1,
    cmd_erase_application = 2,
    cmd_write_data = 3,
    cmd_exit = 4,
    cmd_write_page = 64  // internal commands start at 64
};
register uint8_t command asm("r3");  // bind command to r3

/* ------------------------------------------------------------------------ */
static inline void eraseApplication(void);
static void writeFlashPage(void);
static void writeWordToPageBuffer(uint16_t data);
static uint8_t usbFunctionSetup(uint8_t data[8]);
static inline void leaveBootloader(void);
void blinkLED(uint8_t aBlinkCount);

// This function is never called, it is just here to suppress a compiler warning for old configurations.
USB_PUBLIC usbMsgLen_t __attribute__((unused)) usbFunctionDescriptor(struct usbRequest *rq) {
    (void) rq;
    return 0;
}

/*
 * erase all pages until bootloader, in reverse order (so our vectors stay in place for as long as possible)
 * to minimize the chance of leaving the device in a state where the bootloader wont run, if there's power failure
 * during upload
 */
static inline void eraseApplication(void) {
    uint16_t ptr = BOOTLOADER_ADDRESS; // from Makefile.inc

    while (ptr) {
#if (defined __AVR_ATtiny841__)||(defined __AVR_ATtiny441__)||(defined __AVR_ATtiny1634__)
    ptr -= SPM_PAGESIZE * 4;
#else
        ptr -= SPM_PAGESIZE;
#endif
        boot_page_erase(ptr);
        /*
         * Compiles to:
         * 83 e0           ldi r24, 0x03   ; 3
         * 80 93 57 00     sts 0x0057, r24 ; 0x800057
         * e8 95           spm
         *
         * using the 2 lines instead saves 2 bytes, but then the bootloader does not work :-(
         * __SPM_REG = (__BOOT_PAGE_ERASE);
         * asm volatile("spm" : : "z" ((uint16_t)(ptr))); // the value of ptr is used and can not be optimized away
         */
#if (defined __AVR_ATmega328P__)||(defined __AVR_ATmega168P__)||(defined __AVR_ATmega88P__)||(defined __AVR_ATtiny828__)
    // the ATmegaATmega328p/168p/88p don't halt the CPU when writing to RWW flash, so we need to wait here
    boot_spm_busy_wait();
#endif
    }

    // Reset address to ensure the reset vector is written first.
    currentAddress.w = 0;
}

/*
 * Simply write currently stored page in to already erased flash memory
 */
static inline void writeFlashPage(void) {
    if (currentAddress.w - 2 < BOOTLOADER_ADDRESS) {
        boot_page_write(currentAddress.w - 2);   // will halt CPU, no waiting required
#if (defined __AVR_ATmega328P__)||(defined __AVR_ATmega168P__)||(defined __AVR_ATmega88P__)||(defined __AVR_ATtiny828__)
    // the ATmega328p/168p/88p don't halt the CPU when writing to RWW flash
    boot_spm_busy_wait();
#endif
    }
}

/*
 * Write a word into the page buffer.
 * Will overwrite the bootloader reset vector sent from the host with our fixed value.
 * Normally both values are the same, this is only a safety net if transfer was disturbed
 * to ensure that the device can not be bricked.
 * Handling user-reset-vector is done in the host tool, starting with firmware V2.
 */
static void writeWordToPageBuffer(uint16_t data) {

#ifndef ENABLE_UNSAFE_OPTIMIZATIONS // adds 10 bytes
#  if BOOTLOADER_ADDRESS < 8192
    // rjmp
    if (currentAddress.w == RESET_VECTOR_OFFSET * 2) {
        data = 0xC000 + (BOOTLOADER_ADDRESS / 2) - 1;
    }
#  else
  // far jmp
  if (currentAddress.w == RESET_VECTOR_OFFSET * 2) {
    data = 0x940c;
  } else if (currentAddress.w == (RESET_VECTOR_OFFSET + 1 ) * 2) {
    data = (BOOTLOADER_ADDRESS/2);
  }
  #endif
#endif

#if OSCCAL_SAVE_CALIB
    if (currentAddress.w == BOOTLOADER_ADDRESS - TINYVECTOR_OSCCAL_OFFSET) {
        data = OSCCAL; // save current adjusted OSCCAL value in the postscript area
    }
#endif

    boot_page_fill(currentAddress.w, data);
    currentAddress.w += 2;
}

/*
 * This function is called when the driver receives a SETUP transaction from
 * the host which is not answered by the driver itself (in practice: class and
 * vendor requests). All control transfers start with a SETUP transaction where
 * the host communicates the parameters of the following (optional) data
 * transfer. The SETUP data is available in the 'data' parameter which can
 * (and should) be casted to 'usbRequest_t *' for a more user-friendly access
 * to parameters.
 *
 * If the SETUP indicates a control-in transfer, you should provide the
 * requested data to the driver. There are two ways to transfer this data:
 * (1) Set the global pointer 'usbMsgPtr' to the base of the static RAM data
 * block and return the length of the data in 'usbFunctionSetup()'. The driver
 * will handle the rest.
 *
 * For more explanations see usbdrv.h
 *
 * Prepares variables and sets command for command to be executed in the main loop
 *
 */
static uint8_t usbFunctionSetup(uint8_t data[8]) {
    usbRequest_t *rq = (void*) data;

    idlePolls.b[1] = 0; // reset high byte of idle counter when we get usb class or vendor requests to start a new timeout
    if (rq->bRequest == cmd_device_info) { // get device info
        usbMsgPtr = (usbMsgPtr_t) configurationReply;
#if defined(STORE_CONFIGURATION_REPLY_IN_RAM)
        usbMsgPtrIsRAMAddress = 0;
#endif
        return sizeof(configurationReply);
    } else if (rq->bRequest == cmd_transfer_page) {
        // Set page address. Address zero always has to be written first to ensure reset vector patching.
        // Mask to page boundary to prevent vulnerability to partial page write "attacks"
        if (currentAddress.w != 0) {
            currentAddress.b[0] = rq->wIndex.bytes[0] & (~(SPM_PAGESIZE - 1));
            currentAddress.b[1] = rq->wIndex.bytes[1];

            // Clear temporary page buffer in SRAM as a precaution before filling the buffer
            // in case a previous write operation failed and there is still something in the buffer.
#ifdef CTPB
            __SPM_REG = (_BV(CTPB) | _BV(__SPM_ENABLE));
#else
#ifdef RWWSRE
            __SPM_REG = (_BV(RWWSRE) | _BV(__SPM_ENABLE));
#else
            __SPM_REG=_BV(__SPM_ENABLE);
  #endif
#endif
            asm volatile("spm");

        }
    } else if (rq->bRequest == cmd_write_data) { // Write data
        writeWordToPageBuffer(rq->wValue.word);
        writeWordToPageBuffer(rq->wIndex.word);
        if ((currentAddress.b[0] % SPM_PAGESIZE) == 0) {
            command = cmd_write_page; // ask main loop to write our page
        }
    } else {
        // Handle cmd_erase_application and cmd_exit
        command = rq->bRequest & 0x3f;
    }
    return 0;
}

/*
 * Try to disable watchdog and set timeout to maximum in case the WDT can not be disabled, because it is fused on.
 * Shortest watchdog timeout is 16 ms.
 */
static void inactivateWatchdog(void) {
#ifdef CCP
#ifndef SAVE_MCUSR
    MCUSR = 0;
#endif
    // New ATtinies841/441 use a different unlock sequence and renamed registers
    CCP = 0xD8;
    WDTCSR = _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
#elif defined(WDTCR)
#ifndef SAVE_MCUSR
    MCUSR = 0;
#endif
    WDTCR = _BV(WDCE) | _BV(WDE); // Unlock the watchdog register
    WDTCR = _BV(WDP2) | _BV(WDP1) | _BV(WDP0); // Do not enable watchdog. Set timeout to 2 seconds, just in case, it is fused on and can not disabled.
#else
    wdt_reset();
#ifndef SAVE_MCUSR
    MCUSR = 0;
#endif
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    WDTCSR=0;
#endif
}

/*
 * USB disconnect by disabling pullup resistor by pull down D-, wait 300ms and reconnect
 * Initialize interrupt settings after reconnect but let the global interrupt be disabled
 */
static void reconnectAndInitUSB(void) {
    usbDeviceDisconnect(); // Disable pullup resistor by pull down D-
    _delay_ms(RECONNECT_DELAY_MILLIS); // Even 250 is to fast!
    usbDeviceConnect(); // Enable pullup resistor by changing D- to input

    usbInit();    // Initialize interrupt settings after reconnect but let the global interrupt be disabled
}

/* ------------------------------------------------------------------------ */
// reset system to a normal state and launch user program
__attribute__((__noreturn__)) static inline void leaveBootloader(void) {

    // bootLoaderExit() is a Macro defined in bootloaderconfig.h and mainly empty except for ENTRY_JUMPER, where it resets the pullup.
    bootLoaderExit();

#ifdef SAVE_MCUSR // adds 6 bytes
    GPIOR0 = MCUSR;
    MCUSR = 0;
#endif

#if OSCCAL_RESTORE_DEFAULT
  OSCCAL=osccal_default;
  asm volatile("nop"); // NOP to avoid CPU hickup during oscillator stabilization
#endif

#if (defined __AVR_ATmega328P__)||(defined __AVR_ATmega168P__)||(defined __AVR_ATmega88P__)||(defined __AVR_ATtiny828__)
  // Tell the system that we want to read from the RWW memory again.
  boot_rww_enable();
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
    // jump to application reset vector at end of flash
    asm volatile ("rjmp __vectors - " STR(TINYVECTOR_RESET_OFFSET));
    __builtin_unreachable(); // Tell the compiler function does not return, to help compiler optimize
}

void USB_handler(void); // must match name used in usbconfig.h line 25 and implemented in usbdrvasm.S

int main(void) {

    // bootLoaderInit() is a Macro defined in bootloaderconfig.h and mainly empty except for ENTRY_JUMPER, where it sets the pullup and waits 1 ms.
    bootLoaderInit();

    /* save default OSCCAL calibration  */
#if OSCCAL_RESTORE_DEFAULT
  osccal_default = OSCCAL;
#endif

#if OSCCAL_SAVE_CALIB
    // Adjust clock to previous calibration value, so bootloader AND User program starts with proper clock calibration, even when not connected to USB
    unsigned char stored_osc_calibration = pgm_read_byte(BOOTLOADER_ADDRESS - TINYVECTOR_OSCCAL_OFFSET);
    if (stored_osc_calibration != 0xFF) {
        OSCCAL = stored_osc_calibration;
        // we changed clock so "wait" for one cycle
        asm volatile("nop");
    }
#endif
    // bootLoaderStartCondition() is a Macro defined in bootloaderconfig.h and mainly is set to true or checks a bit in MCUSR
    if (bootLoaderStartCondition() || (pgm_read_byte(BOOTLOADER_ADDRESS - TINYVECTOR_RESET_OFFSET + 1) == 0xff)) {
        /*
         * Here boot condition matches or vector table is empty / no program loaded
         */

        inactivateWatchdog(); // Sets at least watchdog timeout to 2 seconds.

        reconnectAndInitUSB(); // USB disconnect by disabling pullup resistor by pull down D-, wait 300ms and reconnect, and enable USB interrupts

        LED_INIT(); // Set LED pin to output, if LED exists

#if (FAST_EXIT_NO_USB_MS > 120) // At default FAST_EXIT_NO_USB_MS is defined 0
        /*
         * Bias idle counter to wait only for FAST_EXIT_NO_USB_MS if no reset was detected (which is first in USB protocol)
         * Requires 8 bytes. We have to set both registers, since they are undetermined!
         */
        idlePolls.w = ((AUTO_EXIT_MS - FAST_EXIT_NO_USB_MS) / 5);
#else
        // start with 0 to exit after AUTO_EXIT_MS milliseconds (6 seconds) of USB inactivity (not connected or Idle)
        idlePolls.b[1] = 0; // only set register 7, register 6 is almost random (determined by the usage before)
#endif

        command = cmd_local_nop; // initialize register 3
        currentAddress.w = 0;

        /*
         * 1. Wait for 5 ms or USB transmission (and detect reset)
         * 2. Interpret and execute USB command
         * 3. Parse data packet and construct response (usbpoll)
         * 4. Check for timeout and exit to user program
         * 5. Blink LED
         * 6. Resynchronize USB
         */
        do {
            // Adjust t5msTimeoutCounter for 5ms loop timeout. We have 15 clock cycles per loop.
            uint16_t t5msTimeoutCounter = (uint16_t) (F_CPU / (1000.0f * 15.0f / 5.0f));
            uint8_t tResetDownCounter = 100; // start value to detecting reset timing
            /*
             * Now wait for 5 ms or USB transmission
             * If reset from host via USB was detected, then calibrate OSCCAL
             * If USB transmission was detected, reset idle / USB exit counter
             */
            do {
                /*
                 * If host resets us, both lines are driven to low (=SE0)
                 */
                if ((USBIN & USBMASK) != 0) {
                    // No host reset here! We are unconnected with pullup or receive host data -> rearm counter
                    tResetDownCounter = 100;
                }
                if (--tResetDownCounter == 0) {
                    /*
                     * Reset timing counter expired -> reset encountered here!
                     * Both lines were low (=SE0) for around 100 us. Standard says: reset is SE0 ≥ 2.5 ms
                     * I observed 2 Resets. First is 100ms after connecting to USB (above) lasting 65 ms and the second 90 ms later and also 65 ms.
                     */
                    // init 2 V-USB variables as done before in reset handling of usbpoll()
                    usbNewDeviceAddr = 0;
                    usbDeviceAddr = 0;

#if (OSCCAL_HAVE_XTAL == 0)
                    /*
                     * Called if we received an host reset. This waits for the D- line to toggle or at least.
                     * It will wait forever, if no host is connected and the pullup at D- was detached.
                     * In this case we recognize a (dummy) host reset but no toggling at D- will occur.
                     */
                    tuneOsccal();
#endif
#if (FAST_EXIT_NO_USB_MS > 0)
                    // I measured 350 ms (940 ms on my old Linux laptop) from here to the configurationReply request
                    idlePolls.w = ((AUTO_EXIT_MS - 1200) / 5); // Allow another 1200 ms for micronucleus to request configurationReply
#endif
                }

                if (USB_INTR_PENDING & (_BV(USB_INTR_PENDING_BIT))) {
                    USB_handler(); // call V-USB driver for USB receiving
                    USB_INTR_PENDING = _BV(USB_INTR_PENDING_BIT); // Clear int pending, in case timeout occurred during SYNC
                    /*
                     * readme for 2.04 says: If you activate it, idlepolls is only reset when traffic to the current endpoint is detected.
                     * This will let micronucleus timeout also when traffic from other USB devices is present on the bus,
                     * but this leads to periodically reconnecting if no user program is existent. This behavior is like the one of the v1.06 bootloader.
                     */
                    // idlePolls.b[1] = 0; // reset idle polls when we see usb traffic
                    break;
                }

            } while (--t5msTimeoutCounter); // after 5 ms t5msTimeoutCounter is 0.

            asm volatile("wdr");
            // perform cyclically watchdog reset, for the case it is fused on and we can not disable it.

#if OSCCAL_SLOW_PROGRAMMING // reduce clock to enable save flash programming timing
            uint8_t osccal_tmp  = OSCCAL;
            OSCCAL      = osccal_default;
#endif
            /*
             * command is only evaluated here and set by usbFunctionSetup()
             */
            if (command == cmd_erase_application) {
                eraseApplication();
            }
            if (command == cmd_write_page) {
                writeFlashPage();
            }
#if OSCCAL_SLOW_PROGRAMMING
            OSCCAL      = osccal_tmp;
#endif

            if (command == cmd_exit) {
                if (!t5msTimeoutCounter) {
                    break;  // Only exit after 5 ms timeout
                }
            } else {
                command = cmd_local_nop;
            }

            {
                // This is the old usbpoll() minus reset logic (which is done above) and double buffering (not needed anymore)
                // Parse data packet and construct response
                int8_t len;
                len = usbRxLen - 3;

                if (len >= 0) {
                    usbProcessRx(usbRxBuf + 1, len); // only single buffer due to in-order processing
                    usbRxLen = 0; /* mark rx buffer as available */
#if (FAST_EXIT_NO_USB_MS > 0)
                    if ((*(usbRxBuf + 1) & USBRQ_TYPE_MASK) != USBRQ_TYPE_STANDARD) {
                        // we have a request from a running micronucleus program here
                        idlePolls.b[1] = 0; // Reset counter to have 6 seconds timeout since we detected running micronucleus program here
                    }
#endif
                }

                if (usbTxLen & 0x10) { /* transmit system idle */
                    if (usbMsgLen != USB_NO_MSG) { /* transmit data pending? */
                        usbBuildTxBlock();
                    }
                }
            }

            // Increment idle counter at least every 5 ms
            idlePolls.w++;

#if (AUTO_EXIT_MS > 0)
            // Try to execute program when bootloader times out
            if (idlePolls.w == (AUTO_EXIT_MS / 5) && pgm_read_byte(BOOTLOADER_ADDRESS - TINYVECTOR_RESET_OFFSET + 1) != 0xff) {
                break; // Only exit to user program, if program exists
            }
#endif

            // Switch LED on for 4 Idle loops every 64th idle loop - implemented by masking LSByte of idle counter with 0x4C
            // Requires 10 byte, but the compiler requires 12 byte :-(
            LED_MACRO(idlePolls.b[0]);

            // Test whether another interrupt occurred during the processing of USBpoll and commands.
            // If yes, we missed a data packet on the bus. Wait until the bus was idle for 8.8 us to
            // allow synchronizing to the next incoming packet.

            if (USB_INTR_PENDING & (_BV(USB_INTR_PENDING_BIT))) {
                // Usbpoll() collided with data packet
                uint8_t ctr;

                // loop takes 5 cycles
                asm volatile(
                        "         ldi  %0,%1 \n\t"
                        "loop%=:  sbis %2,%3  \n\t"
                        "         ldi  %0,%1  \n\t"
                        "         subi %0,1   \n\t"
                        "         brne loop%= \n\t"
                        : "=&d" (ctr)
                        : "M" ((uint8_t)(8.8f*(F_CPU/1.0e6f)/5.0f+0.5)), "I" (_SFR_IO_ADDR(USBIN)), "M" (USB_CFG_DMINUS_BIT)
                );
                USB_INTR_PENDING = _BV(USB_INTR_PENDING_BIT);
            }
        } while (1);

        /*
         * USB transmission timeout -> cleanup and call user program
         */
        // Set LED pin to low (and input - see below) if LED exists - 2 (sometimes 4) bytes
        LED_EXIT();

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#ifdef USB_CFG_PULLUP_IOPORTNAME
        usbDeviceDisconnect(); // Changing USB_PULLUP_OUT to input() to avoid to drive the pullup resistor, and set level to low.
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
        // with STR(), the compiler is able to optimize the if :-) but gives a warning we can ignore.
        if (STR(USBDDR) == STR(LED_DDR) && LED_MODE != ACTIVE_LOW) {
            USBDDR = 0; // Set all pins to input, including LED and D- pin. The latter keeps device connected.
        } else {
            usbDeviceConnect(); // Changing only D- to input(). This keeps device connected.
        }
#endif
#pragma GCC diagnostic pop

        /*
         * Disable all previously enabled interrupts.
         * The global interrupt will be enabled by the startup code of the application.
         */
        USB_INTR_ENABLE = 0;
        USB_INTR_CFG = 0; /* also reset config bits */

    }

    leaveBootloader();
}

/*
 * For debugging purposes
 */
#if LED_MODE!=NONE
void blinkLED(uint8_t aBlinkCount)
#else
void blinkLED(uint8_t aBlinkCount __attribute__((unused)))
#endif
{
#if LED_MODE!=NONE
    LED_INIT();
    for (uint8_t i = 0; i < aBlinkCount; ++i) {
#  if LED_MODE==ACTIVE_HIGH
        LED_PORT |= _BV(LED_PIN);
#  else
        LED_DDR |= _BV(LED_PIN);
#  endif
        _delay_ms(300);
#  if LED_MODE==ACTIVE_HIGH
        LED_PORT &= ~_BV(LED_PIN);
#  else
        LED_DDR &= ~_BV(LED_PIN);
#  endif
        _delay_ms(300);
    }
    LED_EXIT();
#endif
}
/* ------------------------------------------------------------------------ */
