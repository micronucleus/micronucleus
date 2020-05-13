# Name: Makefile
# Project: Micronucleus
# License: GNU GPL v2 (see License.txt)

# Controller type: ATtiny 85 - 16.5 MHz
# Configuration:   t85_fast_exit_on_no_USB
# Last Change:     4/2020

# Enable safe optimizations. This will disable several unnecesary instructions in microncleus to save around 12 more bytes.
# These optimizations disables entering the bootloader with jmp 0000, which you should not do anyway - better use the watchdog timer reset functionality.
#
# Disabled features:
#   * No SREG initialization in CRT, especially no unnecesary reset of interrupt enable, which is disabled after reset. - saves 2 bytes
#   * No Stackpointer initialization in CRT. Datasheet says: Initial Value RAMEND - saves 8 bytes
#   * Setting the GIMSK or EIMSK register directly instead of keeping other bits. GICR contains other bits, which must be kept! - saves 2 bytes
#     We only want one interrupt to be enabled, so keeping the other bits makes no sense.

# Not needed, since we are not over the page boundary
#CFLAGS += -DENABLE_SAFE_OPTIMIZATIONS  # This saves 10 bytes and disables reliable jump to 0x0000, which one should avoid anyway.

F_CPU = 16500000
DEVICE = attiny85

# hexadecimal address for bootloader section to begin. To calculate the best value:
# - make clean; make main.hex; ### output will list data: 1592 (or something like that)
# - for the size of your device (8kb = 1024 * 8 = 8192) subtract above value = 6598
# - How many pages in is that? 6598 / 64 (tiny85 page size in bytes) = 103.09377
# - round that down to 103
# - our new bootloader address is 103 * 64 = 6592, in hex = 19C0
# - For data size up to 1600 the address is 19C0 for size from 1601 to 1664 the address is 1980
BOOTLOADER_ADDRESS = 19C0

FUSEOPT = -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m
FUSEOPT_DISABLERESET = -U lfuse:w:0xe1:m -U efuse:w:0xfe:m -U hfuse:w:0x5d:m

#---------------------------------------------------------------------
# ATtiny85
#---------------------------------------------------------------------
# Fuse extended byte:
# 0xFE = - - - -   - 1 1 0
#                        ^
#                        |
#                        +---- SELFPRGEN (enable self programming flash)
#
# Fuse high byte:
# 0xdd = 1 1 0 1   1 1 0 1
#        ^ ^ ^ ^   ^ \-+-/
#        | | | |   |   +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (disable external reset -> enabled)
#
# Fuse high byte ("no reset": external reset disabled, can't program through SPI anymore)
# 0x5d = 0 1 0 1   1 1 0 1
#        ^ ^ ^ ^   ^ \-+-/
#        | | | |   |   +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (disable external reset -> disabled!)
#
# Fuse low byte:
# 0xe1 = 1 1 1 0   0 0 0 1
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> HF PLL)
#        | |  +--------------- SUT 1..0 (BOD enabled, fast rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> don't divide)

###############################################################################
