micronucleus tiny85
===================
Micronucleus is a bootloader designed for AVR tiny 85 chips with a minimal usb interface, cross platform libusb-based program upload tool, and a strong emphasis on bootloader compactness. The project aims to release a 2.0kb usb bootloader, and is nearly there, with recent builds at 2.07kb. By using the tinyvector mechanism designed by Embedded Creations in their USBaspLoader-tiny85 project, combined with the simplicity of Objective Development's bootloadHID and a unique bare bones usb protocol, Micronucleus is the smallest usb bootloader available for tiny85 at the time of writing.

Micronucleus adds a small amount of delay to the Pin Change interrupt in user applications, but  this latency is low enough to not interfere with V-USB applications. Once bootloaded, an ISP or HVSP programmer can enable use of the reset pin in user applications. While nobody has yet written an upgrade program, it is theoretically possible to upgrade the bootloader in place by uploading a special upgrade program to it which overwrite the bootloader with a newer version when run.

tiny85 does not offer any hardware bootloading support, and does not protect the bootloader from being accidentally overwritten by a misbehaving app. We recommend great caution if using flash self programming inside an uploaded program due to the potential of bricking. 

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

