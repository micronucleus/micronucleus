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

APP_ADDRESS = 80

CC = avr-gcc

# Remove the -fno-* options when you use gcc 3, it does not understand them
CFLAGS = -Wall -Os -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions -I. -Ilibs-device -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) $(DEFINES)
LDFLAGS = -Wl,--relax,--gc-sections -Wl,--section-start=.text=$(APP_ADDRESS),-Map=upgrade.map

OBJECTS =	upgrade.o bootloader.o

# symbolic targets:
all: upgrade.hex
	

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


clean:
	@rm -f upgrade.hex upgrade.bin upgrade.s upgrade.map upgrade.c.lst upgrade.bin.lss
	@rm -f bootloader.* *.o *~

# file targets:

upgrade.o: upgrade.c bootloader.h

upgrade.bin:	$(OBJECTS) 
	@$(CC) $(CFLAGS) -o upgrade.bin $(OBJECTS) $(LDFLAGS)
	@avr-objdump -d $@ > $@.lss

upgrade.hex:	upgrade.bin
	@rm -f upgrade.hex upgrade.eep.hex
	@avr-objcopy -j .text -j .data -O ihex upgrade.bin upgrade-app.hex
	@cat upgrade-prefix.hex upgrade-app.hex > upgrade.hex
	@rm upgrade-app.hex
	@avr-size upgrade.hex

bootloader.raw: ../firmware/releases/$(CONFIG).hex
	@avr-objcopy -I ihex -O binary $< $@

bootloader.o: bootloader.raw
	@avr-objcopy -I binary -O elf32-avr \
	--rename-section .data=.text \
	--redefine-sym _binary_$*_raw_start=$* \
	--redefine-sym _binary_$*_raw_end=$*_end \
	--redefine-sym _binary_$*_raw_size=$*_size_sym \
	$< $@


bootloader.h: bootloader.o Makefile
	@echo building $@
	@echo "extern const uint8_t" $*"[] PROGMEM;" > $@
	@echo "extern const uint8_t" $*_end"[] PROGMEM;" >> $@
	@echo "extern const uint8_t" $*_size_sym"[];" >> $@
	@echo "#define $*_size ( (int) $*_size_sym )" >> $@
	@echo "#define $*_address 0x$(BOOTLOADER_ADDRESS)" >> $@

