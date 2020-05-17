# Name: Makefile
# Project: Micronucleus
# License: GNU GPL v2 (see License.txt)

# Controller type: ATtiny 85
# Configuration:   Size optimized. Uses 16 MHz V-USB implementation, which may be instable with some computers
# Last Change:     Jan 11,2015

# Enable unsafe optimizations. This will disable several safety features in microncleus to save around 40 more bytes
#
# Disabled features:
#   * Stack pointer and SREG initialization in CRT
#   * Client side reset vector patching
#   * USB collision detection. Micronucleus will not work reliability with hubs if this is disabled.

#CFLAGS += -DENABLE_UNSAFE_OPTIMIZATIONS

F_CPU = 16000000
DEVICE = attiny85

# hexadecimal address for bootloader section to begin. To calculate the best value:
# - make clean; make main.hex; ### output will list data: 1404
# - for the size of your device (8kb = 1024 * 8 = 8192) subtract above value = 6788
# - How many pages in is that? 6770 / 64 (tiny85 page size in bytes) = 106
# - round that down to 106 - our new bootloader address is 106 * 64 = 6784, in hex = 1A40
# - For data size up to 1408 the address is 1A80 for size from 1409 the address is 1A40
BOOTLOADER_ADDRESS = 1A80

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
