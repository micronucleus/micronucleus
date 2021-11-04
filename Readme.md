# Micronucleus V2.6

Micronucleus is a bootloader designed for AVR ATtiny microcontrollers with a minimal usb interface, cross platform libusb-based program upload tool, and a strong emphasis on bootloader compactness. To the authors knowledge this is, by far, the smallest USB bootloader for AVR ATtiny

The V2.0 release is a complete rewrite of the firmware and offers significant improvements over V1.x:

* Support for the entire ATtiny family instead of only ATtiny85.
* Much smaller size. All configurations are below 2kb.
* Interrupt free V-USB: no patching of the user program INT-vector anymore.
* Faster uploads due to new protocol.
* Far jmp also allows using ATtinies with more than 8kb flash.
* Many robustness improvements, such as compatibility to USB hubs and 
   less erratic time out behavior.
 
Due to the many changes, also the upload tool had to be updated. The V2.0 upload tool is backwards compatible to the V1.X tool, though.

The last release of the V1.x can be found here: 

https://github.com/micronucleus/micronucleus/tree/v1.11

# Usage

The bootloader allows uploading of new firmware via USB. In its usual configuration it is invoked at device reset and will identify to the host computer. If no communication is initiated by the host machine within a given time, the bootloader will time out and enter the user program, if one is present. 

For proper timing, the command line tool should to be started on the host computer _before_ the bootloader is invoked.

Windows machines will need to install the libusb drivers found in the /windows_drivers folder.  Clean Micronucleus devices without uploaded userprogram will not time out and allow sufficient time for proper driver installation. Linux and OS X do not require custom drivers.

Windows 10: Installing unsigned drivers became more difficult in Windows 10. Please use the Zadig driver installer as provided in the /windows_driver_installer folder.

Please invoke the command line tool with "micronucleus -help" for a list of available options.

The bootloader resides in the same memory as the user program, since the ATtiny series does not support a protected bootloader section. Therefore, special care has to be taken not to overwrite the bootloader if the user program uses the self programming features. The bootloader will patch itself into the reset vector of the user program. No other interrupt vectors are changed.

# Downloads

The latest release version can be found here:
https://github.com/micronucleus/micronucleus/releases

Archives with snapshots of the latest binaries of commandline tool and firmware can be accessed here:
https://github.com/micronucleus/micronucleus/releases/tag/master-LATEST


# Compiling

Micronucleus can be configured to support all devices of the 'classic' ATtiny series, with the exception of the reduced core ATtiny 4/5/9/10/20/40.

To allow maximum flexibility, micronucleus supports a configuration system. To compile micronucleus with a specific configuration, please invoke the AVR-GCC tool-chain with:

```
     make CONFIG=<config_name>
```
   

The makefile makes use of the gnu binutils and some unix commands such as 'mv' and 'rm'. If you are using Windows, make sure you have proper binaries installed. For example by using the 'Atmel Studio command prompt' or msys/mingw32.

Currently, the following configurations are included and tested. Please check the subfolders /firmware/configurations/ for details. 

| Configuration | Specificity | Size |
| :--- | :--- | :--- |
| t84_default     |   ATtiny84A default configuration     |   1476 bytes |
| t841_default    |   ATtiny841 default configuration     |   1522 bytes |
| t45_default     |   ATtiny45  default configuration     |   1510 bytes |
| t85_default     |   ATtiny85  default configuration     |   1510 bytes |
| t85_aggressive  |   ATtiny85  smaller size - critical   |   1358 bytes |
| t88_default     |   ATtiny88  default configuration     |   1432 bytes |
| t167_default    |   ATtiny167 default (uses xtal)       |   1338 bytes |
| Nanite841       |   Nanite841 firmware                  |   1546 bytes |
| m328p_extclock  |   ATMega328p external clock           |   1490 bytes |
| t4313_default   |   Attiny 4313 default configuration   |   1456 bytes |

Please note that the configuration "t85_aggressive" may be instable under certain circumstances. Please revert to "t85_default" if downloading of user programs fails.

Precompiled hex files of the bootloader can be found in /firmware/releases. The files can be directly programmed using AVRDUDE or other programmers. The folder /firmware/upgrade contains hex files that can be used to upgrade devices with existing version of micronucleus with a newer version. These can be uploaded using an older version micronucleus. 

You can add your own configuration by adding a new folder to /firmware/configurations/. The folder has to contain a customized "Makefile.inc" and "bootloaderconfig.h". Feel free to supply a pull request if you added and tested a previously unsupported device.

If changes to the configuration lead to an increase in bootloader size, it may be necessary to change the bootloader start address. Please consult "Makefile.inc" for details.

Other make options:

```
    make CONFIG=<config_name> fuse   	# Configure fuses
    make CONFIG=<config_name> flash  	# Upload the bootloader using AVRDUDE
```
 
There is also an option to disable the reset line and use it as an I/O. While it may seem tempting to use this feature to make an additional I/O pin available on the ATtiny85, we strongly discourage from doing so, as it led to many issues in the past.

Please _"make clean"_ when switching from one configuration to another.


# Devices using Micronucleus


Micronucleus is widely installed on thousands of open source hardware devices. Please find an incomplete list here: [https://github.com/micronucleus/micronucleus/blob/master/Devices_with_Micronucleus.md](https://github.com/micronucleus/micronucleus/blob/master/Devices_with_Micronucleus.md)
 
# License

This project is released under the GPLv2 license. Code uploaded via the bootloader is not subject to any license.

In addition, we'd like you to consider these points if you intend to sell products using micronucleus:

* Please make your hardware open source. At least the schematic needs to be 
   published according to the license inherited from V-USB.
   
* Your documentation should mention Micronucleus and include a link to the 
   main repository ([https://github.com/micronucleus/](https://github.com/micronucleus/))
   
* Please do not "rebrand" micronucleus by renaming the USB device.
 
* Feel welcome to submit a pull request to include your product in the
   "Devices using Micronucleus"-list. 
   
# Changes 

* v2.0b  June 6th, 2015
    This pull request documents changes leading to V2.0: [https://github.com/micronucleus/micronucleus/pull/43](https://github.com/micronucleus/micronucleus/pull/43)
 
 * v2.01 July 26th, 2015
    This pull request documents changes leading to V2.1: [https://github.com/micronucleus/micronucleus/pull/66](https://github.com/micronucleus/micronucleus/pull/66)
    - Fixes "unknown USB device" issue when devices with <16MHz CPU clock were 
      connected to a USB3.0 port.
    - Fixes one bug that could lead to a deadlock if no USB was connected 
      while the bootloader was active and noise was injected into the floating D+ input.
    - D- line is released before the user program is started, instead of pulling it 
      down. This solves various issues where Micronucleus was not recognized after a 
      reset depending on the duration of the reset button activation. Att: This may 
      lead to a "Unknown device" pop-up in Windows, if the user program does not have 
      USB functionality itself. 
      
* v2.02 August 3rd, 2015
    - Fixes timing bug with Windows 10 USB drivers. Some Win 10 drivers reduce the
      delay between reset and the first data packet to 20 ms. This led to an issue 
      with osccalASM.S, which did not terminate correctly.

* v2.03 February 13th, 2016
    - Added page buffer clearing if a new block transfer is initiated. This fixes a 
      critical, but extremely rare bug that could lead to bricking of the
      device if micronucleus is restarted after an USB error.       
    - #74 Fixed LED_INIT macro so it only modifies the DDR register bit of the LED.
      (Thanks @russdill)

* v2.04 Dec 8th, 2018
    - Merged changed to support ATMega328p by @AHorneffer (#132)    
    - Idlepolls is now only reset when traffic to the current endpoint is detected.
      This will let micronucleus timeout also when traffic from other USB devices
      is present on the bus.

* v2.5 March 5th, 2021
    - Removed zero in version numbering as it confused some people. The minor version number of 
      this release is 5.
    - Huge update by @ArminJo addressing many issues especially for ATTinyCore integration:
      * Improved exit handling for USB_CFG_PULLUP_IOPORTNAME and LED_MODE != ACTIVE_LOW
      * Convert ISR into function and optimized CRC handling to save further bytes
      * added code for FAST_EXIT_NO_USB_MS and START_WITHOUT_PULLUP
      * different MCUSR handling for SAVE_MCUSR
      * end with usbDeviceConnect(); or usbDeviceDisconnect(); or USBDDR = 0; instead of initHardware();
      * Remove redundant code in usbdriver to reduce code size.
    - Automated build system for the command line tool using Github actions thanks to @quinot
    - Static binaries for MacOS and linux command line tool. Thanks @stonehippo
    - Increase available user program size by 64 bytes for most implementations
    - Cumulative minor fixes contributed throughout 2019-2020. Big thanks to everyone!
    - "make release" will now rebuild all release hexfiles. Thanks to @Ho-Ro.
    - Added new osccal code by @nerdralph, significantly reducing code size and fixing a bug
      that led to issues when micronucleus was operated on an USB hub together with other
      low-speed USB devices.
    
* v2.6 November 5th, 2021
    - Added compile flag `STORE_CONFIGURATION_REPLY_IN_RAM`.
    - Added 2 bytes USB config data for Bootloader feature flags and application version.
    - Added `--info` flag for micronucleus executable.
    - Swapped D+ and D- and set `OSCCAL_HAVE_XTAL`for t88 to support MH-ET LIVE Tiny88 boards.
    
# Credits

Firmware:
 * Micronucleus V2.6             (c) 2021 Current maintainers: @ArminJo, @cpldcpu
 * Micronucleus V2.04            (c) 2019 Current maintainers: @cpldcpu, @AHorneffer
 * Micronucleus V2.0x            (c) 2016 Tim Bo"scke - cpldcpu@gmail.com
                                 (c) 2014 Shay Green
 * Original Micronucleus         (c) 2012 Jenna Fox
 * Based on USBaspLoader-tiny85  (c) 2012 Louis Beaudoin
 * Based on USBaspLoader         (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 
Commandline tool:
 * Original commandline tool     (c) 2012 by ihsan Kehribar <ihsan@kehribar.me>
                                 (c) 2012 Jenna Fox
 * Updates for V2.x              (c) 2014 T. Bo"scke

Special Thanks:
 * Objective Development's great V-USB bitbanging usb driver
 * Embedded Creations' pioneering and inspiring USBaspLoader-tiny85
 * Digistump for motivation and contributing the VID/PID pair
 * Numerous supporters for smaller bug fixes and improvements.
