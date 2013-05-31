#
# Makefile stuff common to all ATTiny 861 devices.
#

F_CPU = 12000000
DEVICE=attiny167
LDSCRIPT_SUFFIX=-avr35.x

# Where to put the bootloader:
#
# The bootloader itself must be alligned on a flash  page boundry, but there
# are a few bytes - the space where the applications reset and USB interrupts
# are stored - that must reside in the flash page before the loader.
#
# The tiny861 has a flash page size of 64 bytes and with 2-byte interrupt
# vectors, the tiny tables has 8 bytes.
#
BOOTLOADER_ADDRESS = 37f6
PMEM_WRAP=16k

#
# fuse low byte (lfuse)
#  0xe2 =
#       7:     1 = Clock NOT divided by 8
#       6:     1 = Clock output NOT enabled
#       4-5:  11 = startup time 14ck + 64ms
#       0-3:1111 = external crystal 8- MHz
#
# fuse high byte (hfuse):
#  0xef =
#       7:     1 = external reset NOT disabled
#       6:     1 = debug-wire NOT enabled
#       5:     0 = Serial programming enabled
#       4:     1 = watchdog timer NOT always on
#       3:     1 = chip erase does NOT preserve EEPROM
#       0-2: 111 = Brown-Out detector disabled
#
# fuse extended byte (efuse):
#  0xfe = self-programming enabled
#
FUSEOPT= -U lfuse:w:0xff:m -U hfuse:w:0xd7:m -U efuse:w:0xfe:m

HW_INCLUDE_DIR=$(SRCDIR)/hardware/t167-c12

MICRONUCLEUS_WIRING = 1


