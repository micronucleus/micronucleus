# Name: Makefile
# Project: Micronucleus
# Author: Jenna Fox; portions by Christian Starkjohann, Louis Beaudoin
# Creation Date: 2007-12-10
# Tabsize: 4
# Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
# License: GNU GPL v2 (see License.txt)

CONFIG ?= t85_default
#CONFIG ?= t85_aggressive
#CONFIG ?= t841_default
#CONFIG ?= t167_default

###############################################################################
# Configure the following variables according to your AVR.
# Program the device with
#     make fuse    			# to set the clock generator, boot section size etc.
#     make flash   			# to load the boot loader into flash
#     make disablereset		# use external reset line for IO (CAUTION: this is not easy to enable again, see README) 

CFLAGS =
CONFIGPATH	  = configuration/$(CONFIG)
include $(CONFIGPATH)/Makefile.inc

PROGRAMMER ?= -c USBasp
# PROGRAMMER contains AVRDUDE options to address your programmer

# Tools:
AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE) 
CC = avr-gcc

# Options:
CFLAGS += -I. -g2 -Os # -Wall
CFLAGS += -I$(CONFIGPATH) -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) -DBOOTLOADER_ADDRESS=0x$(BOOTLOADER_ADDRESS) 
CFLAGS += -nostartfiles -ffunction-sections -fdata-sections -fpack-struct -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop

LDFLAGS = -Wl,--relax,--section-start=.text=$(BOOTLOADER_ADDRESS),--gc-sections,-Map=main.map

OBJECTS =  crt1.o usbdrv/usbdrvasm.o usbdrv/oddebug.o main.o 
OBJECTS += osccalASM.o

# symbolic targets:
all: main.hex

.c.o:
	@$(CC) $(CFLAGS) -c $< -o $@ -Wa,-ahls=$<.lst

.S.o:
	@$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	@$(CC) $(CFLAGS) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:main.hex:i -B 20

readflash:
	$(AVRDUDE) -U flash:r:read.hex:i -B 20

fuse:
	$(AVRDUDE) $(FUSEOPT) -B 20
	
disablereset:
	$(AVRDUDE) $(FUSEOPT_DISABLERESET) -B 20

read_fuses:
	$(UISP) --rd_fuses -B 20

clean:
	@rm -f main.hex main.bin main.c.lst main.map main.raw *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s main.lss

# file targets:
main.bin:	$(OBJECTS)
	@$(CC) $(CFLAGS) -o main.bin $(OBJECTS) $(LDFLAGS)

main.hex:	main.bin
	@echo Building Micronucleus configuration: $(CONFIG)
	@rm -f main.hex main.eep.hex
	@avr-objcopy -j .text -j .data -O ihex main.bin main.hex
	@echo Size of sections:
	@avr-size main.bin	
	@echo Size of binary hexfile. Use the "data" size to calculate the bootloader address:
	@avr-size main.hex

upgrade: main.bin
	avr-objcopy -O binary main.bin main.raw
	avr-objcopy -I binary -O elf32-avr \
        --rename-section .data=.text \
        --redefine-sym _binary_main_raw_start=loader \
        --redefine-sym _binary_main_raw_end=loader_end \
		main.raw bootloader_linkable.o	
	
disasm:	main.bin
	@avr-objdump -d -S main.bin >main.lss
