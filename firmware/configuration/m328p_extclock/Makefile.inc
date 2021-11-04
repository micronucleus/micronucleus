# Name: Makefile
# Project: Micronucleus
# License: GNU GPL v2 (see License.txt)

# Controller type: ATmega328p
# Configuration:   Uses 16 MHz V-USB implementation, has also been tested with 12 MHz V-USB implementation
# Last Change:     Dec 7, 2018

# Enable unsafe optimizions. This will disable several safety features in microncleus to save around 40 more bytes
#
# Disabled features:
#   * Stack pointer and SREG initialization in CRT
#   * Client side reset vector patching
#   * USB collision detection. Micronucleus will not work reliability with hubs if this is disabled.

#CFLAGS += -DENABLE_UNSAFE_OPTIMIZATIONS

# Change the following to 12000000 if you want to use the 12 MHz V-USB implementation
F_CPU = 16000000
DEVICE = atmega328p

# hexadecimal address for bootloader section to begin. To calculate the best value:
# - make clean; make main.hex; ### output will list data: 2124 (or something like that)
# - for the size of your device (8kb = 1024 * 8 = 8192) subtract above value 2124... = 6068
# - How many pages in is that? 6068 / 64 (tiny85 page size in bytes) = 94.8125
# - round that down to 94 - our new bootloader address is 94 * 64 = 6016, in hex = 1780
BOOTLOADER_ADDRESS = 7A00

# Note: the bootloader must reside in the space that is marked as bootloader flash space
# in the fuse bits. (The ATmega328 prevents flash programming from code that is not in the
# bootloader space.) But the application may also be in part of that space.

FUSEOPT = -U lfuse:w:0xff:m -U hfuse:w:0xd9:m -U efuse:w:0xfd:m
FUSEOPT_DISABLERESET = -U lfuse:w:0xff:m -U hfuse:w:0x59:m -U efuse:w:0xfd:m
# With brownout detection disabled
#FUSEOPT = -U lfuse:w:0xff:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m
#FUSEOPT_DISABLERESET = -U lfuse:w:0xff:m -U hfuse:w:0x59:m -U efuse:w:0xff:m

#---------------------------------------------------------------------
# ATmega328p
#---------------------------------------------------------------------
# Fuse low byte:
# 0xff = 1 1 1 1   1 1 1 1
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> Ext. Crystal > 8MHz)
#        | |  +--------------- SUT 1..0 (Start up time -> (with CKSEL0) slowly rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> don't divide)
#
# Fuse high byte:
# 0xd9 = 1 1 0 1   1 0 0 1
#        ^ ^ ^ ^   ^ \-/ ^
#        | | | |   |  |  +---- BOOTRST (Select reset vector -> start at address 0x0000)
#        | | | |   |  +------- BOOTSZ 1..0 (Bootloader flash size -> 2048 Bytes)
#        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (disable external reset -> reset is enabled)
#
# Fuse high byte ("no reset": external reset disabled, can't program through SPI anymore)
# 0x59 = 0 1 0 1   1 0 0 1
#        ^ ^ ^ ^   ^ \-/ ^
#        | | | |   |  |  +---- BOOTRST (Select reset vector -> start at address 0x0000)
#        | | | |   |  +------- BOOTSZ 1..0 (Bootloader flash size -> 2048 Bytes)
#        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (disable external reset -> reset is disabled)
#
# Fuse extended byte:
# 0xfd = - - - -   - 1 0 1
#        ^ ^ ^ ^   ^ \-+-/
#        \---------/   +------ BODLEVEL 2,1,0 (brownout trigger level -> brownout detection at 2.7 V)
#             +--------------- Unused
#
# Fuse extended byte:
# 0xff = - - - -   - 1 1 1
#        ^ ^ ^ ^   ^ \-+-/
#        \---------/   +------ BODLEVEL 2,1,0 (brownout detection disabled)
#             +--------------- Unused
###############################################################################
