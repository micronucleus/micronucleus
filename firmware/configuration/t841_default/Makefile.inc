# Name: Makefile
# Project: Micronucleus
# License: GNU GPL v2 (see License.txt)
#
# Controller type: ATtiny 841 
# Configuration:   Default configuration - 12 Mhz RC oscillator
# Last Change:     April 27,2016
 

F_CPU = 12000000
DEVICE = attiny841

# hexadecimal address for bootloader section to begin. To calculate the best value:
# - make clean; make main.hex; ### output will list data: 2124 (or something like that)
# - for the size of your device (8kb = 1024 * 8 = 8192) subtract above value 2124... = 6068
# - How many pages in is that? 6068 / 64 (tiny85 page size in bytes) = 94.8125
# - round that down to 94 - our new bootloader address is 94 * 64 = 6016, in hex = 1780
BOOTLOADER_ADDRESS = 1980

FUSEOPT = -U lfuse:w:0xe2:m -U hfuse:w:0xdd:m -U efuse:w:0xf4:m
FUSEOPT_DISABLERESET =  # Not implemented



