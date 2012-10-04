Micronucleus Upgrade
====================

**WARNING**: Upgrade requires more testing. Don't use it on chips you can't recover by other means. Consider it experimental for now.

Upgrade is a virus-like payload you can upload via micronucleus (or other bootloaders) to install a new version of micronucleus on the target chip. The upgrade program works by copying the contents of a bootloader hex file in to a progmem array, then bricking the device so it doesn't enter the bootloader on reset but instead runs the upgrader program exclusively. Next it erases and rewrites the bootloader in place at the same address the hex file specifies (BOOTLOADER_ADDRESS in the case of micronucleus). Once the bootloader has been rewritten, upgrade rewrites it's own interrupt vector table to point every interrupt including reset straight at the newly uploaded bootloader's interrupt vector table at whichever address it was installed.

The program then emits a beep if a piezo speaker is connected between PB0 and PB1. If you have no speaker, use an LED with positive on PB0 (requires resistor). Premade upgraders are included in releases/ for micronucleus builds. Just upload one in the usual way and wait for the beep. Once you hear the beep, power down the device and turn it back on - it should launch your new bootloader!


Creating an Upgrader
====================

    ruby generate-data.rb my_bootloader.hex
    make clean; make

Next upload the 'upgrade.hex' file generated, via whichever bootloader you're using. If you're using micronucleus and have the command line tool installed: micronucleus --run upgrade.hex

The ruby script requires ruby 1.9 be installed. On Mac OS this is best achieved via Mac Homebrew. The 1.8 version included wont do.


License
=======

Released under BSD license. Have fun!
