# Micronucleus Upgrade - original documentation
Upgrade is a virus-like payload you can upload via micronucleus (or other bootloaders!) to install a new version of micronucleus on the target chip. The upgrade program works by compiling the binary contents of a bootloader hex file in to a progmem array, then running on the chip bricking the device so it doesn't enter any existing bootloader anymore but instead runs the upgrade program exclusively. Next it erases and rewrites the bootloader in place at the same address the hex file specifies (BOOTLOADER_ADDRESS in the case of micronucleus). Once the bootloader has been rewritten, upgrade rewrites it's own interrupt vector table to point every interrupt including reset straight at the newly uploaded bootloader's interrupt vector table at whichever address it was installed.

The program then emits a beep if a piezo speaker is connected between PB1 and GND. If you have no speaker, use a LED with positive on PB1 (requires resistor), Digispark compatible boards have this LED already installed. Premade upgrades are included in releases/ for micronucleus builds. Just upload one in the usual way and wait for the beep or LED flash. Once you hear the beep or see the LED, the chip will automatically reboot and should enumerate over USB as a micronucleus device, ready to accept a new program.

Upgrade has only been tested with micronucleus - use it to upload other bootloaders at your own risk!

## Prerequisits
You'll need avr-gcc. For OS X, it can be obtained from http://www.obdev.at/products/crosspack/download.html

## Creating an Upgrade
Executing 'make' will build 'main.hex' (the bootloader itself) and 'upgrade.hex' (the upgrade program with new bootloader included). Next upload the 'upgrade.hex' file generated in this folder, via whichever bootloader you're using, or an ISP or whatever - everything should work. If you're using micronucleus and have the command line tool installed: micronucleus --run upgrade.hex.<br/>

Pre-built upgrades (based on ./releases/*.hex) are available in directory ./upgrades. Shell script MK_ALL.sh was tested under linux / debian stable.

## License
Released under BSD license. Have fun!


## Technical Details
A summary of how 'upgrade' works


-- build process:

1) Generate the hex files 'main.hex' and 'upgrade.hex' using make:
     make clean; make

   The upgrader hex file is built in the usual way, then combined with a fake interrupt vector
   table in the start of the upgrader. This is necessary because bootloaders like micronucleus
   and Fast Tiny & Mega Bootloader only work with firmwares which begin with an interrupt vector
   table, because of the way they mangle the table to forward some interrupts to themselves.

2) Upload the resulting *upgrade.hex* file to a chip you have some means of recovering. If all
   works correctly, consider now uploading it to other chips which maybe more difficult to recover
   but are otherwise identical.


-- how it works:

Taking inspiration from computer viruses, when upgrade runs it goes through this process:

1) Brick the chip:
   The first thing upgrade does is erase the ISR vector table. Erasing it sets the first page to
   0xFFFF words - creating a NOP sled. If the chip looses power or otherwise resets, it wont enter
   the bootloader, sliding in to the upgrader restarting the process.

2) erase and write bootloader:
   The flash pages for the new bootloader are erased and rewritten from start to finish.

3) install the trampoline:
   The fake ISR table which was erased in step one is now written to - a trampoline is added, simply
   forwarding any requests to the new bootloader's interrupt vector table. At this point the viral
   upgrader has completed it's life cycle and has disabled itself. It should never run again, booting
   directly in to the bootloader instead.

