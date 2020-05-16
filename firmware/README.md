# Compiling
Micronucleus can be configured to support all devices of the ATtiny series, with the exception of the reduced core ATtiny 4/5/9/10/20/40.

To allow maximum flexibility, micronucleus supports a [configuration system](/firmware/configuration). To compile micronucleus with a specific configuration, please invoke the AVR-GCC tool-chain with:

```
make CONFIG=<config_name>
```

Please note that the configuration "t85_aggressive" may be instable unders certain circumstances. Please revert to "t85_default" if downloading of user programs fails.

You can add your own configuration by adding a new folder to /firmware/configurations/. The folder has to contain a customized "Makefile.inc" and "bootloaderconfig.h". 

If changes to the configuration lead to an increase in bootloader size, it may be necessary to [change the bootloader start address](firmware/configuration#computing-the-values).

Other make options:

```
make CONFIG=<config_name> fuse    # Configure fuses
make CONFIG=<config_name> flash   # Uploade the bootloader using AVRDUDE
```

There is also an option to disable the reset line and use it as an I/O. While it may seem tempting to use this feature to make an additional I/O pin available on the ATtiny85, we strongly discourage from doing so, as it led to many issues in the past.

Please "make clean" when switching from one configuration to another.
