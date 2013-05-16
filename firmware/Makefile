# Name: Makefile
# Project: Micronucleus
# Author: Jenna Fox; portions by Christian Starkjohann, Louis Beaudoin
# Creation Date: 2007-12-10
# Tabsize: 4
# Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
# License: GNU GPL v2 (see License.txt)

###############################################################################
# Configure the following variables according to your AVR.
# Program the device with
#     make fuse    			# to set the clock generator, boot section size etc.
#     make flash   			# to load the boot loader into flash
#     make disablereset		# for ATtiny85 target - to use external reset line for IO (CAUTION: this is not easy to enable again, see README)

ALL_HARDWARE=$(basename $(notdir $(wildcard hardware/*.mk)))

ifeq (,$(filter target-%,$(notdir $(CURDIR))))
include target.mk
else

include $(SRCDIR)/hardware/$(HARDWARE).mk

# PROGRAMMER = -c USBasp
PROGRAMMER = -c stk500 -P /dev/ttyACM0
# PROGRAMMER contains AVRDUDE options to address your programmer

# Tools:
AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
CC = avr-gcc

# Options:
DEFINES = -DBOOTLOADER_ADDRESS=0x$(BOOTLOADER_ADDRESS) #-DDEBUG_LEVEL=2
# Remove the -fno-* options when you use gcc 3, it does not understand them
CFLAGS = \
	-Wall \
	-Os \
	-g \
	-fno-move-loop-invariants \
	-fno-tree-scev-cprop \
	-fno-inline-small-functions \
	-I $(HW_INCLUDE_DIR) \
	-I $(SRCDIR) \
	-I $(SRCDIR)/libs-device \
	-nostartfiles \
	-mmcu=$(DEVICE) \
	-DF_CPU=$(F_CPU) \
	$(DEFINES)

LDFLAGS = \
	-Wl,--relax \
	-Wl,--gc-sections

BOOTLOADER_OBJECTS =  usbdrv/usbdrvasm.o bootloader.o

BOOTLOADER_LDFLAGS= \
	-Wl,-Map=bootloader.map \
	-T$(SRCDIR)/bootloader$(LDSCRIPT_SUFFIX) \
	-Wl,--section-start=.text=$(BOOTLOADER_ADDRESS)

VPATH=..

# symbolic targets:
all: bootloader.hex

.c.o:
	@echo "CC  $@ ..."
	@$(CC) $(CFLAGS) -c $< -o $@ -Wa,-ahls=$(notdir $<).lst

.S.o:
	@echo "ASM $@ ..."
	@$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(CC) $(CFLAGS) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:bootloader.hex:i

readflash:
	$(AVRDUDE) -U flash:r:read.hex:i

fuse:
	$(AVRDUDE) $(FUSEOPT)

disablereset:
	$(AVRDUDE) $(FUSEOPT_t85_DISABLERESET)

clean:
	rm -f bootloader.hex bootloader.elf bootloader.c.lst bootloader.map *.o usbdrv/*.o bootloader.s usbdrv/oddebug.s usbdrv/usbdrv.s libs-device/osccal.o

# file targets:
bootloader.elf:	$(BOOTLOADER_OBJECTS)
	@echo "LD  $@"
	@$(CC) $(CFLAGS) -o $@ $(BOOTLOADER_OBJECTS) $(LDFLAGS) $(BOOTLOADER_LDFLAGS)

bootloader.hex:	bootloader.elf
	@echo "HEX $@"
	@avr-objcopy -j .text -j .data -O ihex $< $@
	@avr-size $@

disasm:	bootloader.elf
	avr-objdump -S -d bootloader.elf

cpp:
	$(CC) $(CFLAGS) -E bootloader.c

endif
