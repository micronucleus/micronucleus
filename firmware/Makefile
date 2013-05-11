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

HARDWARE=tiny-85-2.mk
# HARDWARE=tiny-861-c12.mk

include hardware/$(HARDWARE)

PROGRAMMER = -c USBasp
# PROGRAMMER = -c stk500 -P /dev/ttyACM0
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
	-I. -Ilibs-device \
	-I $(HW_INCLUDE_DIR) \
	-nostartfiles \
	-mmcu=$(DEVICE) \
	-DF_CPU=$(F_CPU) \
	$(DEFINES)

LDSCRIPT=my-avr25.x

LDFLAGS = \
	-Wl,--relax \
	-Wl,--gc-sections \
	-Wl,--section-start=.text=$(BOOTLOADER_ADDRESS) \
	-Wl,-Map=main.map \
	-T$(LDSCRIPT)

OBJECTS =  usbdrv/usbdrvasm.o usbdrv/oddebug.o main.o

# symbolic targets:
all: main.hex

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@ -Wa,-ahls=$<.lst

.S.o:
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(CC) $(CFLAGS) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:main.hex:i

readflash:
	$(AVRDUDE) -U flash:r:read.hex:i

fuse:
	$(AVRDUDE) $(FUSEOPT)

disablereset:
	$(AVRDUDE) $(FUSEOPT_t85_DISABLERESET)

clean:
	rm -f main.hex main.bin main.c.lst main.map *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s libs-device/osccal.o

# file targets:
main.bin:	$(OBJECTS)
	$(CC) $(CFLAGS) -o main.bin $(OBJECTS) $(LDFLAGS)

main.hex:	main.bin
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -O ihex main.bin main.hex
	avr-size main.hex

disasm:	main.bin
	avr-objdump -S -d main.bin

cpp:
	$(CC) $(CFLAGS) -E main.c


