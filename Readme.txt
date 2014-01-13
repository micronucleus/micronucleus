Micronucleus 
=============
Micronucleus is a bootloader designed for AVR tiny 85 chips with a minimal usb interface, cross platform libusb-based program upload tool, and a strong emphasis on bootloader compactness. The project aims to release a 2.0kb usb bootloader, and has reached this goal with the latest release. By using the tinyvector mechanism designed by Embedded Creations in their USBaspLoader-tiny85 project, combined with the simplicity of Objective Development's bootloadHID and a unique bare bones usb protocol, Micronucleus is the smallest usb bootloader available for tiny85 at the time of writing.

Micronucleus adds a small amount of delay to the Pin Change interrupt in user applications, but  this latency is low enough to not interfere with V-USB applications. Once bootloaded, an ISP or HVSP programmer can disable the reset pin, offering an extra pin for GPIO and ADC use! After disabling the reset pin functionality, of course you will no longer be able to use ISP programmers with the chip, but that's okay because we made a neat 'upgrade' program. The Upgrade program takes a compiled bootloader hex file and packs it in to an AVR program. You upload the 'upgrade' program via an existing micronucleus installation, any other bootloader, or via ISP or HVSP programmer, and once uploaded the upgrade program runs and writes over the bootloader and then installs a trampoline over it's own interrupt vector table, then reboots, launching the new bootloader. In this way users can change their bootloader to have bugfixes or different configurations like the 'jumper' versions without needing any programming tools.

tiny85 does not offer any hardware bootloading support, and does not protect the bootloader from being accidentally overwritten by a misbehaving app. We recommend great caution if using flash self programming inside an uploaded program due to the potential of bricking.

Micronucleus is now widely installed on over 40,000 Digispark devices from Digistump - a tiny unofficial arduino device, so you can be confident that micronucleus will be well supported in the future. Micronucleus is now also the recommended bootloader for Ihsan Kehribar's wonderful LittleWire devices, and can be successfully installed on to existing LittleWire's by uploading the 'upgrade' program via the old serial bootloader, then uploading the littlewire firmware via the micronucleus command line upload tool.

Changes 
=======

This is release 1.11. Please use this at your own risk. The last official release for the DigiSpark is v1.06, which can be found here: https://github.com/micronucleus/micronucleus/tree/v1.06

Changes compared to v1.10:
 • The size was reduced further to 1816 bytes, allowing 6380 bytes user space (320 bytes more than in v1.06).
 • The bootloader will always start and never quit if no user program was loaded. This allows for much easier driver installation. Use the new "--erase-only" function of the command line tool to create a clean device.
 • New entrymodes have been added. See firmware release notes and source code comments for details.
 • All incoming data is now CRC checked to improve robustness.
 
Changes compared to v1.06:
 • Major size optimization and code reorganization.
 • The size was reduced to 1878 bytes, allowing 6314 bytes user space (256 bytes more than in v1.06).
 • The bootloader will disconnect from USB on exit.

See release notes (/firmware/releases/release notes.txt) for details.
  
@cpldcpu - Jan 14th, 2013

----------------------------------------------------------------------------------
==================================================================================
!!%$!^%%$!#%$@#!%$@!$#@!%$#@%!#@%$!@$%#@!$%%!$#^&%$!%(*$!^%#!$@!#%$*^%!!&^%!%@$#!^
@#$%^&*%#$%^#($)#*&($^#^*%&%%&@$*#($^&^*@$#&%^*%&($^&#^*%&$(^^%@$&^*#%@%&$^#*%^*%&
&$%#$&^&$%@&#$*^*##*$##^$&#^%$^&*$&^&^%$#^%$&*$&#^%$*^$#^&%$*%#^$&^*%$#^$*^$$&*%#$
==================================================================================
----------------------------------------------------------------------------------

Special Thanks:
 • Shay Green for numerous optimization ideas.
 • Objective Development's great V-USB bitbanging usb driver
 • Embedded Creations' pioneering and inspiring USBaspLoader-tiny85
 • Digistump for motivation and contributing the VID/PID pair
 • Ihsan Kehribar for the command line C-based upload tool

This project is released under the GPLv2 license. Code uploaded via the bootloader is not subject to any license issues.

