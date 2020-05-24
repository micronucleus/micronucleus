# Micronucleus USB Bootloader for ATtinies / Digisparks
### Version 2.5
[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](https://www.gnu.org/licenses/gpl-2.0)
[![Hit Counter](https://hitcounter.pythonanywhere.com/count/tag.svg?url=https://github.com/ArminJo/micronucleus-firmware)](https://github.com/brentvollebregt/hit-counter)

Micronucleus is a bootloader designed for AVR ATtiny microcontrollers with a minimal usb interface, cross platform libusb-based program upload tool, and a strong emphasis on bootloader compactness. To the authors knowledge this is, by far, the smallest USB bootloader for AVR ATtiny.<br/>
**The V2.0 release is a complete rewrite of the firmware and offers significant improvements over V1.x.**<br/>
Due to the many changes, also the upload tool had to be updated. The V2.0 upload tool is backwards compatible to the V1.X tool, though.

![Digisparks](pictures/Digisparks.jpg)

# Usage
The bootloader allows **uploading of new firmware via USB**. In its usual configuration it is invoked at device power or on reset and will identify to the host computer. If no communication is initiated by the host machine within a given time (default are 6 seconds), the bootloader will time out and enter the user program, if one is present.<br/>
For proper timing, the command line tool should to be started on the host computer **before** the bootloader is invoked / the board attached.<br/>
The bootloader resides in the same memory as the user program, since the ATtiny series does not support a protected bootloader section. Therefore, special care has to be taken not to overwrite the bootloader if the user program uses the self programming features. The bootloader will patch itself into the reset vector of the user program. **No other interrupt vectors are changed**.<br/>
Please invoke the command line tool with "micronucleus -help" for a list of available options.

# Driver installation
For Windows you must install the **libusb driver** before you can program the board. Download it [here](https://github.com/digistump/DigistumpArduino/releases/download/1.6.7/Digistump.Drivers.zip), open it and run `InstallDrivers.exe`.
Clean Micronucleus devices without uploaded userprogram will not time out and allow sufficient time for proper driver installation. Linux and OS X do not require custom drivers.

# Installation of a better optimizing compiler configuration for Digispark boards
**The new Digistump AVR version 1.6.8 shrinks generated code size by 5 to 15 percent**. Just replace the old Digispark board URL **http://digistump.com/package_digistump_index.json** (e.g.in Arduino *File/Preferences*) by the new  **https://raw.githubusercontent.com/ArminJo/DigistumpArduino/master/package_digistump_index.json** and install the **Digistump AVR Boards** version **1.6.8**.<br/>
![Boards Manager](https://github.com/ArminJo/DigistumpArduino/blob/master/pictures/Digistump1.6.8.jpg)<br/>

# Update the bootloader to the new version
To **update** your old flash consuming **bootloader**, jou have 2 choices.
1. Install the new Digistump board manager (see above), open the Arduino IDE, select *Tools/Programmer: "Micronucleus"* and then run *Tools/Burn Bootloder*.<br/>
![Burn Bootloader](https://github.com/ArminJo/DigistumpArduino/blob/master/pictures/Micronucleus_Burn_Bootloader.jpg)<br/>
The bootloader is the recommended configuration [`entry_on_power_on_no_pullup_fast_exit_on_no_USB`](firmware/configuration#recommended-configuration).<br/>
2. Run one of the Windows [scripts](https://github.com/ArminJo/micronucleus-firmware/tree/master/utils)
like e.g. the [Burn_upgrade-t85_default.cmd](utils/Burn_upgrade-t85_default.cmd).

# Configuration overview is [here](firmware/configuration#overview)

# New features
## MCUSR content now available in sketch
In this versions the reset flags in the MCUSR register are no longer cleared by micronucleus and can therefore read out by the sketch!<br/>
If you use the flags in your program or use the `ENTRY_POWER_ON` boot mode, **you must clear them** with `MCUSR = 0;` **after** saving or evaluating them.
If you do not reset the flags, and use the `ENTRY_POWER_ON` mode of the bootloader, the bootloader will be entered even after a reset, since the power on reset flag in MCUSR is still set!<br/>
For `ENTRY_EXT_RESET` configuration see [Fixed wrong ENTRY_EXT_RESET].

# Bootloader memory comparison of different releases for [*t85_default.hex*](firmware/releases/t85_default.hex).
- V1.6  6012 bytes free
- V1.11 6330 bytes free
- V2.3  6522 bytes free
- V2.04 6522 bytes free
- V2.5  **6586** bytes free

## This repository contains also an avrdude config file in `windows_exe` with **ATtiny87** and **ATtiny167** data added.

# Pin layout
### ATtiny85 on Digispark

```
                       +-\/-+
 RESET/ADC0 (D5) PB5  1|    |8  VCC
  USB- ADC3 (D3) PB3  2|    |7  PB2 (D2) INT0/ADC1 - default TX Debug output for ATtinySerialOut
  USB+ ADC2 (D4) PB4  3|    |6  PB1 (D1) MISO/DO/AIN1/OC0B/OC1A/PCINT1 - (Digispark) LED
                 GND  4|    |5  PB0 (D0) OC0A/AIN0
                       +----+
  USB+ and USB- are each connected to a 3.3 volt Zener to GND and with a 68 Ohm series resistor to the ATtiny pin.
  On boards with a micro USB connector, the series resistor is 22 Ohm instead of 68 Ohm. 
  USB- has a 1k pullup resistor to indicate a low-speed device (standard says 1k5).                  
  USB+ and USB- are each terminated on the host side with 15k to 25k pull-down resistors.
```

### ATtiny167 on Digispark pro
Digital Pin numbers in braces are for ATTinyCore library

```
                  +-\/-+
RX   6 (D0) PA0  1|    |20  PB0 (D8)  0 OC1AU
TX   7 (D1) PA1  2|    |19  PB1 (D9)  1 OC1BU - (Digispark) LED
     8 (D2) PA2  3|    |18  PB2 (D10) 2 OC1AV
INT1 9 (D3) PA3  4|    |17  PB3 (D11) 4 OC1BV USB-
           AVCC  5|    |16  GND
           AGND  6|    |15  VCC
    10 (D4) PA4  7|    |14  PB4 (D12)   XTAL1
    11 (D5) PA5  8|    |13  PB5 (D13)   XTAL2
    12 (D6) PA6  9|    |12  PB6 (D14) 3 INT0  USB+
     5 (D7) PA7 10|    |11  PB7 (D15)   RESET
                  +----+
  USB+ and USB- are each connected to a 3.3 volt Zener to GND and with a 51 Ohm series resistor to the ATtiny pin.
  USB- has a 1k5 pullup resistor to indicate a low-speed device.
  USB+ and USB- are each terminated on the host side with 15k to 25k pull-down resistors.

```

# Devices using Micronucleus
Micronucleus is widely installed on thousands of open source hardware devices. Please find an incomplete list [here](https://github.com/micronucleus/micronucleus/blob/master/Devices_with_Micronucleus.md)

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
