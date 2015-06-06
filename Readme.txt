Micronucleus V2.0b
=============
Micronucleus is a bootloader designed for AVR ATtiny microcontrollers with a minimal usb interface, cross platform libusb-based program upload tool, and a strong emphasis on bootloader compactness. To the authors knowledge this is, by far, the smallest USB bootloader for AVR ATtiny

The V2.0 release is a complete rewrite of the firmware and offers significant improvements over V1.x:

 • Support for the entire ATtiny family instead of only ATtiny85
 • Much smaller size. All configurations are below 2kb.
 • Interrupt free V-USB does not require patching of the user program INT-vector anymore.
 • Faster uploads due to new protocol.
 • Far jmp also allows using ATtinies with more than 8kb flash.
 • Many robustness improvements, such as compatibility to USB hubs and less erratic time out behavior.
 
Due to the many changes, also the uploadtool had to be updated. The V2.0 upload tool is backwards compatible to the V1.X tool, though.

The last release of the V1.x can be found here: https://github.com/micronucleus/micronucleus/tree/v1.11

Usage
=====

The bootloader allows uploading of new firmware via USB. In its usual configuration it is invoked at device reset and will identify to the host computer. If no comminucation is initiated by the host machine within a given time, the bootloader will time out and enter the user program, if one is present. 

For proper timing, the commmand line tool should to be started on the host computer _before_ the bootloader is invoked.

Windows machines will need to install the libusb drivers found in the /windows_drivers folder.  Clean Micronucleus devices without uploaded userprogram will not time out and allow suffucient time for proper driver installation. Linux and OS X do not require custom drivers.

Please invoke the command line tool with "microcleus -help" for a list of available options.

The bootloader resides in the same memory as the user program, since the ATtiny series does not support a protected bootloader section. Therefore, special care has to be taken not to overwrite the bootloader if the user programm uses the self programming features. The bootloader will patch itself into the reset vector of the user program. No other interrupt vectors are changed.

Compiling
=========

Micronucleus can be configured to support all devices of the ATtiny series, with the expection of the reduced core ATtiny 4/5/9/10/20/40.

To allow maximum flexibility, micronucleus supports a configuration system. To compile micronucleus with a specific configuratiion, please invoke the AVR-GCC toolchain with:

    make CONFIG=<config_name>

Currently the following configurations are included and tested. Please check /firmware/configurations/ for details. Hex files can be found in /releases.

t84_default     -   ATtiny84A default configuration     -   1532 bytes
t841_default    -   ATtiny841 default configuration     -   1584 bytes
t85_default     -   ATtiny85  default configuration     -   1606 bytes
t85_aggressive  -   ATtiny85  smaller size - critical   -   1414 bytes
t167_default    -   ATtiny167 default (uses xtal)       -   1390 bytes
Nanite841       -   Nanite841 firmware                  -   1608 bytes

You can add your own configuration by adding a new folder to /firmware/configurations/. The folder has to contain a customized "Makefile.inc" and "boorloaderconfig.h". The bootloader options are explaines in these files. Feel free to supply a pull request if you added and tested a previously unsupported device.

If changes to the configuration lead to an increase in bootloader size, it may be necessary to change the bootloader start address. Please consult "Makefile.inc" for details.

Other make options:

    make CONFIG=<config_name> fuse   	# to set the clock generator, boot section size etc.
    make CONFIG=<config_name> flash  	# to load the boot loader into flash using avrdude
    
There is also an option to disable the reset line and use it as an I/O. While it may seem tempting to use this feature to make available an additional I/O pin available on the ATtiny85, we strongly discourage from doing so, as it led to many issues in the past.

Please "make clean" when switching from one configuration to another.

Devices using Micronucleus
==========================

Micronucleus is widely installed on thousands of open source hardware devices. Please find an incomplete list here https://github.com/micronucleus/micronucleus/Devices_with_Microncleus.md

License
=======

This project is released under the GPLv2 license. Code uploaded via the bootloader is not subject to any license issues.






Micronucleus adds a small amount of delay to the Pin Change interrupt in user applications, but  this latency is low enough to not interfere with V-USB applications. Once bootloaded, an ISP or HVSP programmer can disable the reset pin, offering an extra pin for GPIO and ADC use! After disabling the reset pin functionality, of course you will no longer be able to use ISP programmers with the chip, but that's okay because we made a neat 'upgrade' program. The Upgrade program takes a compiled bootloader hex file and packs it in to an AVR program. You upload the 'upgrade' program via an existing micronucleus installation, any other bootloader, or via ISP or HVSP programmer, and once uploaded the upgrade program runs and writes over the bootloader and then installs a trampoline over it's own interrupt vector table, then reboots, launching the new bootloader. In this way users can change their bootloader to have bugfixes or different configurations like the 'jumper' versions without needing any programming tools.

tiny85 does not offer any hardware bootloading support, and does not protect the bootloader from being accidentally overwritten by a misbehaving app. We recommend great caution if using flash self programming inside an uploaded program due to the potential of bricking.

Micronucleus is now widely installed on over 40,000 Digispark devices from Digistump - a tiny unofficial arduino device, so you can be confident that micronucleus will be well supported in the future. Micronucleus is now also the recommended bootloader for Ihsan Kehribar's wonderful LittleWire devices, and can be successfully installed on to existing LittleWire's by uploading the 'upgrade' program via the old serial bootloader, then uploading the littlewire firmware via the micronucleus command line upload tool.

Changes 
=======

This pull request documents changes leading to V2.0: https://github.com/micronucleus/micronucleus/pull/43


Credits
=======

Firmware:

  • Micronucleus V2.0             (c) 2015 Tim Bo"scke - cpldcpu@gmail.com
                                  (c) 2014 Shay Green
  • Original Micronucleus         (c) 2012 Jenna Fox
 
  • Based on USBaspLoader-tiny85  (c) 2012 Louis Beaudoin
  • Based on USBaspLoader         (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 
Commandline tool:
 
  • Original commandline tool     (c) 2012 by ihsan Kehribar <ihsan@kehribar.me>
  • Updates for V2.x              (c) 2014 T. Bo"scke
  
Special Thanks:

 • Aaron Stone/@sodabrew for building the OS X command line tool and various fixes.
 • Objective Development's great V-USB bitbanging usb driver
 • Embedded Creations' pioneering and inspiring USBaspLoader-tiny85
 • Digistump for motivation and contributing the VID/PID pair
 