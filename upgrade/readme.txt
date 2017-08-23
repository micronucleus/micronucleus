Micronucleus Upgrade
====================

Upgrade is a virus-like payload you can upload via micronucleus (or other bootloaders!) to install a new version of micronucleus on the target chip. The upgrade program works by compiling the binary contents of a bootloader hex file in to a progmem array, then running on the chip bricking the device so it doesn't enter any existing bootloader anymore but instead runs the upgrade program exclusively. Next it erases and rewrites the bootloader in place at the same address the hex file specifies (BOOTLOADER_ADDRESS in the case of micronucleus). Once the bootloader has been rewritten, upgrade rewrites it's own interrupt vector table to point every interrupt including reset straight at the newly uploaded bootloader's interrupt vector table at whichever address it was installed.

The program then emits a beep if a piezo speaker is connected between PB0 and PB1. If you have no speaker, use an LED with positive on PB0 (requires resistor). Premade upgrades are included in releases/ for micronucleus builds. Just upload one in the usual way and wait for the beep. Once you hear the beep, the chip will automatically reboot and should enumerate over USB as a micronucleus device, ready to accept a new program.

Upgrade has only been tested with micronucleus - use it to upload other bootloaders at your own risk!

Prerequesits
============

You'll need avr-gcc. For OS X, it can be obtained from http://www.obdev.at/products/crosspack/download.html


Creating an Upgrade
===================

    ruby generate-data.rb my_new_bootloader.hex
    make clean; make

Next upload the 'upgrade.hex' file generated in this folder, via whichever bootloader you're using, or an ISP or whatever - everything should work. If you're using micronucleus and have the command line tool installed: micronucleus --run upgrade.hex

The generate-data.rb script requires Ruby 1.9 or newer to be installed. If you're using an older version of Mac OS X (before Mavericks), use homebrew to install ruby with 'brew install ruby' to get a recent version. On linuxes you can usually find a package called ruby1.9 in whichever installing thingy. On windows you're on your own!

Pre-built upgrades (based on ../firmware/releases/*.hex) are available in directory ./releases. Shell script MAKE_RELEASES.sh was tested under linux / debian stable.

License
=======

Released under BSD license. Have fun!
