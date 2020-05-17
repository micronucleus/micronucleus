# Overview
If not otherwise noted, the OSCCAL value is calibrated (+/- 1%) after boot for all ATtiny85 configurations
| Configuration | Available FLASH | Bootloader size | Non default config flags set |
|---------------|-----------------|-----------------|------------------------------|
| t85_aggressive | 6780 | 1392 | [Do not provide calibrated OSCCAL, if no USB attached](t85_aggressive/bootloaderconfig.h#L220).<br/>Relying on calibrated 16MHz internal clock stability, do not use the 16.5 MHz USB driver version with integrated PLL. |
|  |  |  |  |
| t85_default | 6586 | 1544 | - |
| t85_entry_on_power_on | 6586 | 1580 | [ENTRY_POWER_ON](#entry_power_on-entry-condition), LED_MODE=ACTIVE_HIGH |
| t85_entry_on_power_on_no_pullup | 6586 | 1594 | [ENTRY_POWER_ON](#entry_power_on-entry-condition), [START_WITHOUT_PULLUP](#start_without_pullup), LED_MODE=ACTIVE_HIGH |
| t85_entry_on_power_on_fast_exit_on_no_USB | 6586 | 1588 | [ENTRY_POWER_ON](#entry_power_on-entry-condition), [START_WITHOUT_PULLUP](#start_without_pullup), [FAST_EXIT_NO_USB_MS=300](#fast_exit_no_usb_ms-for-fast-bootloader-exit), LED_MODE=ACTIVE_HIGH |
| **t85_entry_on_power_on_no_pullup_fast_exit_on_no_USB**<br/>[recommended configuration](#recommended-configuration) | 6586 | 1584 | [ENTRY_POWER_ON](#entry_power_on-entry-condition), [START_WITHOUT_PULLUP](#start_without_pullup), [FAST_EXIT_NO_USB_MS=300](#fast_exit_no_usb_ms-for-fast-bootloader-exit) |
| t85_entry_on_power_on_pullup_at_0 | 6586 | 1568 | [ENTRY_POWER_ON](#entry_power_on-entry-condition), USB_CFG_PULLUP_IOPORTNAME + USB_CFG_PULLUP_BIT |
| t85_fast_exit_on_no_USB | 6586 | 1570 | [FAST_EXIT_NO_USB_MS=300](#fast_exit_no_usb_ms-for-fast-bootloader-exit), LED_MODE=ACTIVE_HIGH |
| t85_entry_on_reset_no_pullup | 6582 | 1600 | [ENTRY_EXT_RESET](#entry_ext_reset-entry-condition), [START_WITHOUT_PULLUP](#start_without_pullup), LED_MODE=ACTIVE_HIGH |
|  |  |  |  |
| t167_default | 14970 | 1350 | - |
| t167_entry_on_power_on_no_pullup | 14842 | 1388 | [ENTRY_POWER_ON](#entry_power_on-entry-condition), [START_WITHOUT_PULLUP](#start_without_pullup), LED_MODE=ACTIVE_HIGH |
| **t167_entry_on_power_on_no_pullup_fast_exit_on_no_USB**<br/>[recommended configuration](#recommended-configuration) | 14842 | 1396 | [ENTRY_POWER_ON](#entry_power_on-entry-condition), [START_WITHOUT_PULLUP](#start_without_pullup), [FAST_EXIT_NO_USB_MS=300](#fast_exit_no_usb_ms-for-fast-bootloader-exit), LED_MODE=ACTIVE_HIGH |
| t167_entry_on_reset_no_pullup | 14842 | 1396 | [ENTRY_EXT_RESET](#entry_ext_reset-entry-condition), [START_WITHOUT_PULLUP](#start_without_pullup), LED_MODE=ACTIVE_HIGH |
|  |  |  |  |
| Nanite841 |  | 1594 |  |
| BitBoss |  | 1588 |  |
| t84_default |  | 1520 |  |
|  |  |  |  |
| m168p_extclock |  | 1510 |  |
| m328p_extclock |  | 1510 |  |

### Legend
- [ENTRY_POWER_ON](#entry_power_on-entry-condition) - Only enter bootloader on power on, not on reset or brownout.
- [ENTRY_EXT_RESET](#entry_ext_reset-entry-condition) - Only enter bootloader on reset, not on power up.
- [FAST_EXIT_NO_USB_MS=300](#fast_exit_no_usb_ms-for-fast-bootloader-exit) - If not connected to USB (e.g. powered via VIN) the sketch starts after 300 ms (+ initial 300 ms) -> 600 ms.
- [START_WITHOUT_PULLUP](#start_without_pullup) - Bootloader does not hang up, if no pullup is activated/connected.
- [ENABLE_SAFE_OPTIMIZATIONS](#enable_safe_optimizations) - jmp 0x0000 does not initialize the stackpointer.
- [LED_MODE=ACTIVE_HIGH](https://github.com/ArminJo/micronucleus-firmware/blob/eebe73c46e7780d52c92e6f1c00c72edc26b7769/firmware/main.c#L527) - The built in LED flashes during the 5 seconds of the bootloader waiting for commands.

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
## [`ENABLE_SAFE_OPTIMIZATIONS`](https://github.com/ArminJo/micronucleus-firmware/tree/master/firmware/crt1.S#L99).
This configuration is specified in the [Makefile.inc](t85_fast_exit_on_no_USB/Makefile.inc#L18) file and will disable several unnecesary instructions in microncleus. These optimizations disables reliable entering the bootloader with `jmp 0x0000`, which you should not do anyway - better use the watchdog timer reset functionality.<br/>
This is enabled for [t85_entry_on_power_on](t85_entry_on_power_on), [t85_fast_exit_on_no_USB](t85_fast_exit_on_no_USB) and [t85_pullup_at_0](t85_pullup_at_0). It brings no benefit for other configurations.<br/>
- Gains 10 bytes.

## [`FAST_EXIT_NO_USB_MS`](t85_fast_exit_on_no_USB/bootloaderconfig.h#L184) for fast bootloader exit
If the bootloader is entered, it requires 300 ms to initialize USB connection (disconnect and reconnect). 
100 ms after this 300 ms initialization, the bootloader receives a reset, if the host application wants to program the device.<br/>
This enable us to wait for 200 ms after initialization for a reset and if no reset detected to exit the bootloader and start the user program. 
So the user program is started with a 500 ms delay after power up (or reset) even if we do not specify a special entry condition.<br/>
The 100 ms time to reset may depend on the type of host CPU etc., so I took 200 ms to be safe. 

## [`ENTRY_POWER_ON`](t85_entry_on_power_on/bootloaderconfig.h#L108) entry condition
The `ENTRY_POWER_ON` configuration adds 18 bytes to the ATtiny85 default configuration, but this behavior is **what you normally need** if you use a Digispark board, since it is programmed by attaching to the USB port resulting in power up.<br/>
In this configuration **a reset will immediately start your sketch** without any delay.<br/>
Do not forget to **reset the flags in setup()** with `MCUSR = 0;` to make it work!<br/>

## [`ENTRY_EXT_RESET`](t85_entry_on_reset_no_pullup/bootloaderconfig.h#L122) entry condition
The ATtiny85 has the bug, that it sets the `External Reset Flag` also on power up. To guarantee a correct behavior for `ENTRY_EXT_RESET` condition, it is checked if only this flag is set **and** all MCUSR flags are **always reset** before start of user program. The latter is done to avoid bricking the device by fogetting to reset the `PORF` flag in the user program.<br/>
The content of the MCUSR is copied to the OCR1C register before clearing the flags. This enables the user program to interprete it.<br/>
For ATtiny167 it is even worse, it sets the `External Reset Flag` and the `Brown-out Reset Flag` also on power up. Here the MCUSR content is copied to the ICR1L register before clearing.<br/>

## [`START_WITHOUT_PULLUP`](t85_entry_on_power_on_no_pullup_fast_exit_on_no_USB/bootloaderconfig.h#L207)
The `START_WITHOUT_PULLUP` configuration adds 16 to 18 bytes for an additional check. It is required for low energy applications, where the pullup is directly connected to the USB-5V and not to the CPU-VCC. Since this check was contained by default in all pre 2.0 versions, it is obvious that **it can also be used for boards with a pullup**.

## [Recommended](t85_entry_on_power_on_no_pullup_fast_exit_on_no_USB) configuration
The recommended configuration is *entry_on_power_on_no_pullup_fast_exit_on_no_USB*:
- Entry on power on, no entry on reset, ie. after a reset the application starts immediately.
- Start even if pullup is disconnected. Otherwise the bootloader hangs forever, if you commect the Pullup to USB-VCC to save power.
- Fast exit of bootloader (after 600 ms) if there is no host program sending us data (to upload a new application/sketch).

#### Hex files for these configuration are already available in the [releases](/firmware/releases) and [upgrades](/firmware/upgrades) folders.

## Create your own configuration
You can easily create your own configuration by adding a new *firmware/configuration* directory and adjusting *bootloaderconfig.h* and *Makefile.inc*. Before you run the *firmware/make_all.cmd* script, check the arduino directory path in the [`firmware/SetPath.cmd`](/firmware/SetPath.cmd#L1) file.<br/>
Feel free to supply a pull request if you added and tested a previously unsupported device.
