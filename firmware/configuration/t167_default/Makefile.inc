# Name: Makefile
# Project: Micronucleus
# License: GNU GPL v2 (see License.txt)
#
# Controller type: ATtiny 167
# Configuration:   Default configuration.
# Last Change:     Mar 16,2014


F_CPU = 16000000
DEVICE = attiny167
AVR_ARCHITECTURE_PARAMETER = --binary-architecture avr:35

# hexadecimal address for bootloader section to begin. To calculate the best value:
# - make clean; make main.hex; ### output will list data: 1390 (or something like that)
# - for the size of your device (16kb = 1024 * 16 = 16384) subtract above value = 14994
# - How many pages in is that? 14994 / 128 (tiny167 page size in bytes) = 117.140
# - round that down to 117 - our new bootloader address is 117 * 128 = 14976, in hex 3A80
# - For data from 1281 to 1408 the address is 3A80 for size from 1409 to 1536 the address is 3A00
BOOTLOADER_ADDRESS = 3A80

FUSEOPT = # TODO
FUSEOPT_DISABLERESET = # TODO
