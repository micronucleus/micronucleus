# Micronucleus USB Bootloader for ATtiny Microcontrollers
### Version 2.5
[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](https://www.gnu.org/licenses/gpl-2.0)
[![Hit Counter](https://hitcounter.pythonanywhere.com/count/tag.svg?url=https://github.com/ArminJo/micronucleus-firmware)](https://github.com/brentvollebregt/hit-counter)

Micronucleus is a bootloader designed for AVR ATtiny microcontrollers with a minimal usb interface, cross platform libusb-based program upload tool, and a strong emphasis on bootloader compactness. To the authors knowledge this is, by far, the smallest USB bootloader for AVR ATtiny.

The V2.0 release is a complete rewrite of the firmware and offers significant improvements over V1.x. Due to the many changes, also the upload tool had to be updated. The V2.0 upload tool is backwards compatible to the V1.X tool.

# Devices using Micronucleus
Micronucleus is widely installed on thousands of open source hardware devices. Please find an incomplete list [here](https://github.com/micronucleus/micronucleus/blob/master/Devices_with_Micronucleus.md)

# Usage

*Please take a look at the [Digispark section](Digispark_usage.md) for specifics about the Arduino environment.*

The bootloader allows uploading of new firmware via USB. In its usual configuration it is invoked at device power on or reset and will identify to the host computer. If no communication is initiated by the host machine within a given time (default are 6 seconds), the bootloader will time out and enter the user program, if one is present.

For proper timing, the command line tool should to be started on the host computer **before** the bootloader is invoked or the board isattached.

The bootloader resides in the same memory as the user program, since the ATtiny series does not support a protected bootloader section. Therefore, special care has to be taken not to overwrite the bootloader if the user program uses the self programming features. The bootloader will patch itself into the reset vector of the user program. No interrupt vectors are changed.

Please invoke the command line tool with "micronucleus -help" for a list of available options.

# Driver installation

For Windows you must install the libusb driver before you can program the board (See /windows_driver and /windows_driver_installer). Linux and OS X do not require custom drivers.

Clean Micronucleus devices without uploaded userprogram will not time out and allow sufficient time for proper driver installation. You can create a clean micronucleus device by erasing the userprogram using ```micronculeus --erase-only```

# Compiling

Micronucleus can be configured to support all devices of the ATtiny series, with the exception of the reduced core ATtiny 4/5/9/10/20/40.

To allow maximum flexibility, micronucleus supports a configuration system. To compile micronucleus with a specific configuration, please invoke the AVR-GCC tool-chain with:

    make CONFIG=<config_name>

Other make options:

    make CONFIG=<config_name> fuse   	# Configure fuses
    make CONFIG=<config_name> flash  	# Uploade the bootloader using AVRDUDE

Please refer to the [configuration overview](firmware/configuration#overview) for further detailss.

## The MCUSR content is now available in the userprogram

Starting with version 2.5, the reset flags in the MCUSR register are no longer cleared by micronucleus and can therefore be read out by the userprogram. If you use the flags in your program or use the `ENTRY_POWER_ON` boot mode, you must clear the flags with `MCUSR = 0;` after saving or evaluating them. If you do not reset the flags and use the `ENTRY_POWER_ON` mode of the bootloader, the bootloader will be entered even after a reset, since the power on reset flag in MCUSR is still set.
For `ENTRY_EXT_RESET` configuration see [Fixed wrong ENTRY_EXT_RESET].

# Revision History
### Version 2.5
- Saved 2 bytes by removing for loop at leaveBootloader().
- Saved 2 bytes by defining __DELAY_BACKWARD_COMPATIBLE__ for _delay_ms().
- Saved 22 bytes by converting USB_INTR_VECTOR in *usbdrvasm165.inc* from ISR with pushes to a plain function.
- Saved 2 bytes by improving small version of usbCrc16 in *usbdrvasm.S*.
- Saved 4 bytes by inlining usbCrc16 in *usbdrvasm.S*.
- Renamed `AUTO_EXIT_NO_USB_MS` to `FAST_EXIT_NO_USB_MS` and implemented it.
- New configurations using `FAST_EXIT_NO_USB_MS` set to 300 ms.
- Light refactoring and added documentation.
- No USB disconnect at bootloader exit. This avoids "Unknown USB Device" entry in device manager.
- Gained 128 byte for `t167_default` configuration.
- Fixed destroying bootloader for upgrades with entry condition `ENTRY_EXT_RESET`.
- Fixed wrong condition for t85 `ENTRYMODE==ENTRY_EXT_RESET`.
- ATtiny167 support with MCUSR bug/problem at `ENTRY_EXT_RESET` workaround.
- `MCUSR` handling improved.
- no_pullup targets for low energy applications forever loop fixed.
- `USB_CFG_PULLUP_IOPORTNAME` disconnect bug fixed.
- New `ENTRY_POWER_ON` configuration switch, which enables the program to start immediately after a reset.

### Version v2.04 Dec 8th, 2018
- Merged changed to support ATMega328p by @AHorneffer (#132)
- Idlepolls is now only reset when traffic to the current endpoint is detected. This will let micronucleus timeout also when traffic from other USB devices
  is present on the bus.

### Version v2.3 February 13th, 2016
- Added page buffer clearing if a new block transfer is initiated. 
This fixes a critical, but extremely rare bug that could lead to bricking of the device if micronucleus is restarted after an USB error.   
- #74 Fixed LED_INIT macro so it only modifies the DDR register bit of the LED. (Thanks @russdill)

### Version v2.2 August 3rd, 2015
- Fixes timing bug with Windows 10 USB drivers. Some Win 10 drivers reduce the delay between reset and the first data packet to 20 ms.
This led to an issue with osccalASM.S, which did not terminate correctly.

### Version v2.1 July 26th, 2015
This pull request documents changes leading to V2.1: https://github.com/micronucleus/micronucleus/pull/66
- Fixes "unknown USB device" issue when devices with <16MHz CPU clock were connected to a USB3.0 port.
- Fixes one bug that could lead to a deadlock if no USB was connected while the bootloader was active and noise was injected into the floating D+ input.
- D- line is released before the user program is started, instead of pulling it down.
This solves various issues where Micronucleus was not recognized after a reset depending on the duration of the reset button activation.
Att: This may lead to a "Unknown device" pop-up in Windows, if the user program does not have USB functionality itself. 

### Version v2.0b  June 6th, 2015
This pull request documents changes leading to V2.0: https://github.com/micronucleus/micronucleus/pull/43
- Support for the entire ATtiny family instead of only ATtiny85.
- Much smaller size. All configurations are below 2 kb.
- Interrupt free V-USB: no patching of the user program INT-vector anymore.
- Faster uploads due to new protocol.
- Far jmp also allows using ATtinies with more than 8 kb flash.
- Many robustness improvements, such as compatibility to USB hubs and less erratic time out behavior.

### Version 1.11
The last release of the V1.x can be found [here](https://github.com/micronucleus/micronucleus/tree/v1.11).

# License
This project is released under the GPLv2 license. Code uploaded via the bootloader is not subject to any license.
In addition, we'd like you to consider these points if you intend to sell products using micronucleus:
- Please make your hardware open source. At least the schematic needs to be published according to the license inherited from V-USB.
- Your documentation should mention Micronucleus and include a link to the main repository (https://github.com/micronucleus/)
- Please do not "rebrand" micronucleus by renaming the USB device.
- Feel welcome to submit a pull request to include your product in the "Devices using Micronucleus"-list. 

# Credits

## Firmware:
- Micronucleus V2.5             (c) 2020 Current maintainer: @ArminJo
- Micronucleus V2.04            (c) 2019 Current maintainers: @cpldcpu, @AHorneffer
- Micronucleus V2.0x            (c) 2016 Tim Bo"scke - cpldcpu@gmail.com, (c) 2014 Shay Green
- Original Micronucleus         (c) 2012 Jenna Fox
- Based on USBaspLoader-tiny85  (c) 2012 Louis Beaudoin
- Based on USBaspLoader         (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 
## Commandline tool:
- Original commandline tool     (c) 2012 by ihsan Kehribar <ihsan@kehribar.me>, Jenna Fox
- Updates for V2.x              (c) 2014 T. Bo"scke

## Special Thanks:
- Aaron Stone/@sodabrew for building the OS X command line tool and various fixes.
- Objective Development's great V-USB bitbanging usb driver
- Embedded Creations' pioneering and inspiring USBaspLoader-tiny85
- Digistump for motivation and contributing the VID/PID pair
- Numerous supporters for smaller bug fixes and improvements.

