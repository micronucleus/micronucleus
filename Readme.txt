USBaspLoader-tiny85 is a modification to the USBaspLoader project giving 
support for the ATtiny85 microcontroller.  This is a signification modification
as the tiny85 has very limited boot loader support in the hardware.  The 
boot loader becomes a part of the application it has loaded by taking over the
application's reset and USB-related interrupt vectors, yet the application 
does not need to be modified to support the boot loader.  The boot loader does
take up more flash (about 2.8k) than the standard USBaspLoader (about 2k), so
the space available for the application is smaller.


WORKING WITH THE BOOT LOADER
============================
Because of the limited pin count on the tiny85, the boot loader doesn't look
for an external condition to enter boot loader mode, it does it automatically
before starting the application.  On reset the boot loader enumerates with the PC
as a USBasp programmer, and jumps to the application after 5 seconds if there are
no USBasp commands sent by the PC.  You can flash the device with AVRDUDE 
through a "virtual" USBasp programmer. If there is no valid application loaded
(determined by verifying a checksum calculation across application program space)
the boot loader will stay connected as a USBasp programmer until the flash is 
written.


HARDWARE CONFIGURATIONS
=======================
Even through the ATtiny85 only has 6 IO pins, there are a couple ways designers have 
connected the USB signals to the tiny85.  

Configuration 1
The first configuration this bootloader supported was the configuration used by the 
V-USB example "EasyLogger", and by Sparkfun in their AVR Stick: D- is connected to PB0
and D+ is connected to PB2 (INT0).  This is convenient as INT0 can be used for USB, 
and the Pin Change interrupts can be left free for the application to use, but 
inconvienient as INT0 shares the same pin as USCK/SCL preventing use of the USI peripheral 
in SPI or I2C mode.  Also, PB0 is used for MOSI/DI/SDA, preventing use of the USI 
peripheral in UART mode as well as SPI and I2C.

Configuration 2
An alternate configuration, used by Little Wire, puts the D+ pin on PB4 and D- on PB3, 
and uses Pin Change interrupts instead of INT0 for USB.  This allows the USI peripheral
to be used by the application, but prevents using external interrupts: Pin Change 
interrupts are used by USB, and INT0 shouldn't be used as it's a higher priority 
interrupt than Pin Change, and will interfere with USB communication.

(There's a third configuration used by vusbtiny: similar to configuration 2 but with the
D- and D+ pins swapped.  I don't plan to support that configuration in the precompiled 
binaries, though it can be generated easily by changing bootloaderconfig.h and recompiling.)


For more information on the USBaspLoader-tiny85, please visit the
following URL:

http://www.embeddded-creations.com

(c) 2012 Louis Beaudoin

Below is the unmodified README file for USBaspLoader:

=========================================================================

This is the README file for USBaspLoader.

USBaspLoader is a USB boot loader for AVR microcontrollers. It can be used on
all AVRs with at least 2 kB of boot loader section, e.g. the popular ATMega8.
The firmware is flashed into the upper 2 kB of the flash memory and takes
control immediately after reset. If a certain hardware condition is met
(this condition can be configured, e.g. a jumper), the boot loader waits for
data on the USB interface and loads it into the remaining part of the flash
memory. If the condition is not met, control is passed to the loaded firmware.

This boot loader is similar to Thomas Fischl's avrusbboot and our own
bootloadHID, but it requires no separate command line tool to upload the data.
USBaspLoader emulates Thomas Fischl's USBasp programmer instead. You can thus
use AVRDUDE to upload flash memory data (and if the option is enabled) EEPROM
data.

Since USBaspLoader cooperates with AVRDUDE, it can be used in conjunction with
the Arduino software to upload flash memory data.


FILES IN THE DISTRIBUTION
=========================
Readme.txt ........ The file you are currently reading.
firmware .......... Source code of the controller firmware.
firmware/usbdrv ... USB driver -- See Readme.txt in that directory for info
License.txt ....... Public license (GPL2) for all contents of this project.
Changelog.txt ..... Logfile documenting changes in soft-, firm- and hardware.


BUILDING AND INSTALLING
=======================
This project can be built on Unix (Linux, FreeBSD or Mac OS X) or Windows.

For all platforms, you must first describe your hardware in the file
"firmware/bootloaderconfig.h". See the documentation in the example provided
with this distribution for details. Then edit "firmware/Makefile" to reflect
the target device, the device's boot loader address and fuse bit values.

Building on Windows:
You need WinAVR for the firmware, see http://winavr.sourceforge.net/.
To build the firmware with WinAVR, change into the "firmware" directory,
check whether you need to edit the "Makefile" (e.g. change device settings,
programmer hardware, clock rate etc.) or bootloaderconfig.h and type "make"
to compile the source code. Before you upload the code to the device with
"make flash", you should set the fuses with "make fuse". To protect the boot
loader from overwriting itself, set the lock bits with "make lock" after
uploading the firmware.

Building on Unix (Linux, FreeBSD and Mac):
You need the GNU toolchain and avr-libc for the firmware. See
    http://www.nongnu.org/avr-libc/user-manual/install_tools.html
for a good description on how to install the GNU compiler toolchain and
avr-libc on Unix. For Mac OS X, we provide a read-made package, see
    http://www.obdev.at/avrmacpack/

To build the firmware, change to the "firmware" directory, edit "Makefile"
and bootloaderconfig.h as described in the Windows paragraph above and type
"make" to compile the source code. Before you upload the code to the device
with "make flash", you should set the fuses with "make fuse". Then protect the
boot loader firmware with "make lock".


WORKING WITH THE BOOT LOADER
============================
The boot loader is quite easy to use. Set the jumper (or whatever condition
you have configured) for boot loading on the target hardware, connect it to
the host computer and (if not bus powered) issue a Reset on the AVR.

You can now flash the device with AVRDUDE through a "virtual" USBasp
programmer.


USING THE USB DRIVER FOR YOUR OWN PROJECTS
==========================================
This project is not intended as a reference implementation. If you want to
use AVR-USB in your own projects, please see
   * PowerSwitch for the most basic example,
   * Automator for an HID example or
   * AVR-Doper for a very complex example on how to simulate a serial
     interface (virtual COM port).
All these projects can be downloaded from http://www.obdev.at/avrusb/


ABOUT THE LICENSE
=================
It is our intention to make our USB driver and this demo application
available to everyone. Moreover, we want to make a broad range of USB
projects and ideas for USB devices available to the general public. We
therefore want that all projects built with our USB driver are published
under an Open Source license. Our license for the USB driver and demo code is
the GNU General Public License Version 2 (GPL2). See the file "License.txt"
for details.

If you don't want to publish your source code under the GPL2, you can simply
pay money for AVR-USB. As an additional benefit you get USB PIDs for free,
licensed exclusively to you. See the file "CommercialLicense.txt" in the usbdrv
directory for details.


MORE INFORMATION
================
For more information about Objective Development's firmware-only USB driver
for Atmel's AVR microcontrollers please visit the URL

    http://www.obdev.at/products/avrusb/

A technical documentation of the driver's interface can be found in the
file "firmware/usbdrv/usbdrv.h".


--
(c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH.
http://www.obdev.at/
