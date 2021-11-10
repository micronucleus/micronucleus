# Compiling the bootloader firmware
Micronucleus can be configured to support all devices of the ATtiny series, with the exception of the reduced core ATtiny 4/5/9/10/20/40.

To allow maximum flexibility, micronucleus supports a [configuration system](/firmware/configuration). 
To compile micronucleus with a specific configuration, please invoke the AVR-GCC tool-chain with:

```sh
make CONFIG=<config_name>
```

You'll need avr-gcc. For OS X, it can be obtained from http://www.obdev.at/products/crosspack/download.html.
For Linux install the compiler, library and tools with your package system, e.g. for Debian or Ubuntu use

```sh
sudo apt-get install -y gcc-avr avr-libc binutils-avr
```

This also works with [Ubuntu WSL](https://ubuntu.com/wsl) on [Windows 10](https://docs.microsoft.com/windows/wsl/install-win10).

Please note that the configuration "t85_agressive" may be instable unders certain circumstances. Please revert to "t85_default" if downloading of user programs fails.

You can add your own configuration by adding a new folder to /firmware/configurations/. The folder has to contain a customized "Makefile.inc" and "bootloaderconfig.h". 

If changes to the configuration lead to an increase in bootloader size, i.e. you see errors like `address 0x2026 of main.bin section '.text' is not within region 'text'`,
it may be necessary to [change the bootloader start address](https://github.com/ArminJo/micronucleus-firmware#computing-the-bootloader-start-address).

Other make options:

```sh
make CONFIG=<config_name> fuse    # Configure fuses
make CONFIG=<config_name> flash   # Upload the bootloader using AVRDUDE
```

There is also an option to disable the reset line and use it as an I/O. While it may seem tempting to use this feature to make an additional I/O pin available on the ATtiny85, we strongly discourage from doing so, as it led to many issues in the past.

Please "make clean" when switching from one configuration to another.

The following options need to install and configure [avrdude](http://savannah.nongnu.org/projects/avrdude): `flash`, `readflash`,  `fuse`,  `disablereset`,  `read_fuses`:

```sh
sudo apt-get install -y avrdude
```

Alternatively, *avrdude* can be run on Windows using the settings provided by Arduino.

Typical command to upgrade the micronucleus with Windows:

```bat
<path>\micronucleus.exe /run upgrade.hex
```

Typical command to flash the bootloader with Windows (`<COM port>` can be e.g. COM6) using a STK500 compatible programmer like an [Arduino configured as ISP programmer](https://www.arduino.cc/en/Tutorial/BuiltInExamples/ArduinoISP):

```bat
<path>\avrdude.exe -C <path>\avrdude.conf -v -pattiny85 -cstk500v1 -P<COM port> -b19200 -Uflash:w:main.hex:i
```
