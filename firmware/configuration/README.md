# Overview

## Computing the values
The actual memory footprint for each configuration can be found in the file [*firmware/build.log*](firmware/build.log).<br/>
Bytes used by the mironucleus bootloader can be computed by taking the data size value in *build.log*, 
rounding it up to the next multiple of the page size which is e.g. 64 bytes for ATtiny85 and 128 bytes for ATtiny176.<br/>
Subtracting this (+ 6 byte for postscript) from the total amount of memory will result in the free bytes numbers.
- Postscript are the few bytes at the end of programmable memory which store tinyVectors.

E.g. for *t85_default.hex* with the new compiler we get 1548 as data size. The next multiple of 64 is 1600 (25 * 64) => (8192 - (1600 + 6)) = **6586 bytes are free**.<br/>
In this case we have 52 bytes left for configuration extensions before using another 64 byte page.<br/>
For *t167_default.hex* (as well as for the other t167 configurations) with the new compiler we get 1436 as data size. The next multiple of 128 is 1536 (12 * 128) => (16384 - (1536 + 6)) = 14842 bytes are free.<br/>

# Configuration Options

## [`FAST_EXIT_NO_USB_MS`](t85_fast_exit_on_no_USB/bootloaderconfig.h#L184) for fast bootloader exit
If the bootloader is entered, it requires 300 ms to initialize USB connection (disconnect and reconnect). 
100 ms after this 300 ms initialization, the bootloader receives a reset, if the host application wants to program the device.<br/>
This enable us to wait for 200 ms after initialization for a reset and if no reset detected to exit the bootloader and start the user program. 
So the user program is started with a 500 ms delay after power up (or reset) even if we do not specify a special entry condition.<br/>
The 100 ms time to reset may depend on the type of host CPU etc., so I took 200 ms to be safe. 

## [`ENTRY_POWER_ON`](t85_entry_on_power_on/bootloaderconfig.h#L108) entry condition
The `ENTRY_POWER_ON` configuration adds 18 bytes to the ATtiny85 default configuration, but this behavior is **what you normally need** if you use a Digispark board, since it is programmed by attaching to the USB port resulting in power up.<br/>
In this configuration **a reset will immediately start your userprogram** without any delay.<br/>
Do not forget to **reset the flags in setup()** with `MCUSR = 0;` to make it work!<br/>

## [`ENTRY_EXT_RESET`](t85_entry_on_reset_no_pullup/bootloaderconfig.h#L122) entry condition
The ATtiny85 has the bug, that it sets the `External Reset Flag` also on power up. To guarantee a correct behavior for `ENTRY_EXT_RESET` condition, it is checked if only this flag is set **and** all MCUSR flags are **always reset** before start of user program. The latter is done to avoid bricking the device by fogetting to reset the `PORF` flag in the user program.<br/>
The content of the MCUSR is copied to the OCR1C register before clearing the flags. This enables the user program to interprete it.<br/>
For ATtiny167 it is even worse, it sets the `External Reset Flag` and the `Brown-out Reset Flag` also on power up. Here the MCUSR content is copied to the ICR1L register before clearing.<br/>

## [`START_WITHOUT_PULLUP`](t85_entry_on_power_on_no_pullup_fast_exit_on_no_USB/bootloaderconfig.h#L207)
The `START_WITHOUT_PULLUP` configuration adds 16 to 18 bytes for an additional check. It is required for low energy applications, where the pullup is directly connected to the USB-5V and not to the CPU-VCC. Since this check was contained by default in all pre 2.0 versions, it is obvious that **it can also be used for boards with a pullup**.

## [`ENABLE_SAFE_OPTIMIZATIONS`](https://github.com/ArminJo/micronucleus-firmware/tree/master/firmware/crt1.S#L99)
This configuration is specified in the [Makefile.inc](t85_fast_exit_on_no_USB/Makefile.inc#L18) file and will [disable the restoring of the stack pointer](firmware/crt1.S#L102) at the start of program, whis is normally done by reset anyway. These optimization disables reliable entering the bootloader with `jmp 0x0000`, which you should not do anyway - better use the watchdog timer reset functionality.<br/>
- Gains 10 bytes.

## [`ENABLE_UNSAFE_OPTIMIZATIONS`](https://github.com/ArminJo/micronucleus-firmware/tree/master/firmware/crt1.S#L99)
- Includes [`ENABLE_SAFE_OPTIMIZATIONS`](#enable_safe_optimizations).
- The bootloader reset vector is written by the host and not by the bootloader itself. In case of an disturbed communication the reset vector may be wrong -but I have never experienced it.

You have a slightly bigger chance to brick the bootloader, which reqires it to be reprogrammed by [avrdude](windows_exe) -command files can be found [here](utils)- and an ISP or an Arduino as ISP.

#### Hex files for these configuration are already available in the [releases](/firmware/releases) and [upgrades](/firmware/upgrades) folders.

## Create your own configuration
You can easily create your own configuration by adding a new *firmware/configuration* directory and adjusting *bootloaderconfig.h* and *Makefile.inc*. Before you run the *firmware/make_all.cmd* script, check the arduino directory path in the [`firmware/SetPath.cmd`](/firmware/SetPath.cmd#L1) file.<br/>
If changes to the configuration lead to an increase in bootloader size, it may be necessary to change the bootloader start address as described [above](#computing-the-values) or in the *Makefile.inc*.
Feel free to supply a pull request if you added and tested a previously unsupported device.
