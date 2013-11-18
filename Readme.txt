micronucleus testing branch
===========================

This is the current development branch for micronucleus. The focus for this branch is to reduce the size without changing the functionality of the client tool. Please look at individual commits to understand the changes that were made.

* Nov 18,2013 @cpldcpu - 1966 bytes

micronucleus tiny85
===================
Micronucleus is a bootloader designed for AVR tiny 85 chips with a minimal usb interface, cross platform libusb-based program upload tool, and a strong emphasis on bootloader compactness. The project aims to release a 2.0kb usb bootloader, and is nearly there, with recent builds at 2.07kb. By using the tinyvector mechanism designed by Embedded Creations in their USBaspLoader-tiny85 project, combined with the simplicity of Objective Development's bootloadHID and a unique bare bones usb protocol, Micronucleus is the smallest usb bootloader available for tiny85 at the time of writing.

Micronucleus adds a small amount of delay to the Pin Change interrupt in user applications, but  this latency is low enough to not interfere with V-USB applications. Once bootloaded, an ISP or HVSP programmer can disable the reset pin, offering an extra pin for GPIO and ADC use! After disabling the reset pin functionality, of course you will no longer be able to use ISP programmers with the chip, but that's okay because we made a neat 'upgrade' program. The Upgrade program takes a compiled bootloader hex file and packs it in to an AVR program. You upload the 'upgrade' program via an existing micronucleus installation, any other bootloader, or via ISP or HVSP programmer, and once uploaded the upgrade program runs and writes over the bootloader and then installs a trampoline over it's own interrupt vector table, then reboots, launching the new bootloader. In this way users can change their bootloader to have bugfixes or different configurations like the 'jumper' versions without needing any programming tools.

tiny85 does not offer any hardware bootloading support, and does not protect the bootloader from being accidentally overwritten by a misbehaving app. We recommend great caution if using flash self programming inside an uploaded program due to the potential of bricking.

Micronucleus is now widely installed on over 40,000 Digispark devices from Digistump - a tiny unofficial arduino device, so you can be confident that micronucleus will be well supported in the future. Micronucleus is now also the recommended bootloader for Ihsan Kehribar's wonderful LittleWire devices, and can be successfully installed on to existing LittleWire's by uploading the 'upgrade' program via the old serial bootloader, then uploading the littlewire firmware via the micronucleus command line upload tool.

----------------------------------------------------------------------------------
==================================================================================
!!%$!^%%$!#%$@#!%$@!$#@!%$#@%!#@%$!@$%#@!$%%!$#^&%$!%(*$!^%#!$@!#%$*^%!!&^%!%@$#!^
@#$%^&*%#$%^#($)#*&($^#^*%&%%&@$*#($^&^*@$#&%^*%&($^&#^*%&$(^^%@$&^*#%@%&$^#*%^*%&
&$%#$&^&$%@&#$*^*##*$##^$&#^%$^&*$&^&^%$#^%$&*$&#^%$*^$#^&%$*%#^$&^*%$#^$*^$$&*%#$
==================================================================================
----------------------------------------------------------------------------------

Special Thanks:
 • Objective Development's great V-USB bitbanging usb driver
 • Embedded Creations' pioneering and inspiring USBaspLoader-tiny85
 • Digistump for motivation and contributing the VID/PID pair
 • Ihsan Kehribar for the command line C-based upload tool

This project is released under the GPLv2 license. Code uploaded via the bootloader is not subject to any license issues.

