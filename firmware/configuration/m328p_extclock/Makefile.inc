# Name: Makefile
# Project: Micronucleus
# License: GNU GPL v2 (see License.txt)

# Controller type: ATmega328p
# Configuration:   Size optimized. Uses 16 MHz V-USB implementation, which may be instable with some computers
# Last Change:     Nov 14, 2017

# Enable unsafe optimizions. This will disable several safety features in microncleus to save around 40 more bytes
# 
# Disabled features:
#   * Stack pointer and SREG initialization in CRT
#   * Client side reset vector patching
#   * USB collision detection. Micronucleus will not work reliability with hubs if this is disabled.

#CFLAGS += -DENABLE_UNSAFE_OPTIMIZATIONS

F_CPU = 16000000
DEVICE = atmega328p

# hexadecimal address for bootloader section to begin. To calculate the best value:
# - make clean; make main.hex; ### output will list data: 2124 (or something like that)
# - for the size of your device (8kb = 1024 * 8 = 8192) subtract above value 2124... = 6068
# - How many pages in is that? 6068 / 64 (tiny85 page size in bytes) = 94.8125
# - round that down to 94 - our new bootloader address is 94 * 64 = 6016, in hex = 1780
BOOTLOADER_ADDRESS = 7800

FUSEOPT = -U lfuse:w:0xff:m -U hfuse:w:0xda:m -U efuse:w:0xff:m
FUSEOPT_DISABLERESET = -U lfuse:w:0xff:m -U hfuse:w:0x5a:m -U efuse:w:0xff:m

#---------------------------------------------------------------------
# ATMega328p
#---------------------------------------------------------------------
# Fuse extended byte:
# 0xFF = - - - -   - 1 1 1
#                        
#
# Fuse high byte:
# 0xda = 1 1 0 1   1 0 1 0
#
# Fuse low byte:
# 0xff = 1 1 1 1   1 1 1 1
###############################################################################
