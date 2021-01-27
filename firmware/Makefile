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
CONFIGBASEPATH	  = configuration
CONFIGPATH	  = $(CONFIGBASEPATH)/$(CONFIG)
include $(CONFIGPATH)/Makefile.inc

PROGRAMMER ?= -c USBasp
# PROGRAMMER contains AVRDUDE options to address your programmer

# Tools:
AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE) 
CC = avr-gcc

# Options:
CFLAGS += -I. -g2 -Os -Wall -Wextra
CFLAGS += -I$(CONFIGPATH) -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) -DBOOTLOADER_ADDRESS=0x$(BOOTLOADER_ADDRESS) 
CFLAGS += -nostartfiles -ffunction-sections -fdata-sections -fpack-struct -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop

LDFLAGS = -Wl,--relax,--section-start=.text=$(BOOTLOADER_ADDRESS),--gc-sections,-Map=main.map

OBJECTS =  crt1.o usbdrv/usbdrvasm.o usbdrv/oddebug.o main.o 
OBJECTS += osccalASM.o


# symbolic targets:
all: main.hex upgrade.hex

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
	$(AVRDUDE) -B 20

clean:
	@rm -f main.hex main.bin main.c.lst main.map main.raw *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s usbdrv/oddebug.c.lst main.lss main.lst
	@rm -f bootloader.* upgrade.hex upgrade.bin upgrade.c.lst upgrade.map main.s upgrade.lss upgrade.lst upgrade.lss

distclean: clean
	@rm *~


# file targets:
main.bin:	$(OBJECTS)
	@$(CC) $(CFLAGS) -o main.bin $(OBJECTS) $(LDFLAGS)
	@avr-objdump -d -S main.bin > main.lss


main.hex:	main.bin
	@echo Building Micronucleus configuration: $(CONFIG)
	@rm -f main.hex main.eep.hex
	@avr-objcopy -j .text -j .data -O ihex main.bin main.hex
	@echo Size of binary hexfile. Use the "data" size to calculate the bootloader address:
	@avr-size main.hex
# Output to logfile
	@echo Building Micronucleus configuration: $(CONFIG) >> build.log
	@echo Size of binary hexfile. Use the "data" size to calculate the bootloader address: >> build.log
	@avr-size main.hex >> build.log
	@echo - >> build.log



disasm:	main.bin upgrade.bin
	@avr-objdump -d -S main.bin >main.lss
	@avr-nm -l -S -n main.bin > main.lst
	@avr-objdump -d -S upgrade.bin > upgrade.lss
	@avr-nm -l -S -n upgrade.bin > upgrade.lst

APP_ADDRESS = 80

# Remove the -fno-* options when you use gcc 3, it does not understand them
CFLAGS_U = -Wall -g2 -Os -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions -I. -Ilibs-device -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) $(DEFINES) -DBOOTLOADER_ADDRESS=0x$(BOOTLOADER_ADDRESS)
LDFLAGS_U = -Wl,--relax,--gc-sections -Wl,--section-start=.text=$(APP_ADDRESS),-Map=upgrade.map


upgrade.o: upgrade.c bootloader.h
	@$(CC) $(CFLAGS_U) -c $< -o $@ $(LDFLAGS_U) -Wa,-ahls=$<.lst


upgrade.bin:	upgrade.o bootloader.o
	@$(CC) $(CFLAGS_U) -o upgrade.bin upgrade.o bootloader.o $(LDFLAGS_U)
	@avr-objdump -d -S upgrade.bin > upgrade.lss


upgrade.hex:	upgrade.bin
	@rm -f upgrade.hex upgrade.eep.hex
	@avr-objcopy -j .text -j .data -O ihex upgrade.bin upgrade-app.hex
# for internal echo command, mainly on windows
ifeq ($(OS),Windows_NT)
	@echo :1000000003C003C003C003C003C003C003C003C0D8> upgrade.hex
else
# for unix echo command
	@echo ":1000000003C003C003C003C003C003C003C003C0D8" > upgrade.hex
endif
	@cat upgrade-app.hex >> upgrade.hex
	@rm upgrade-app.hex
	@avr-size upgrade.hex


bootloader.raw: main.hex
	@avr-objcopy -I ihex -O binary $< $@


bootloader.o: bootloader.raw
	@avr-objcopy -I binary -O elf32-avr \
	--rename-section .data=.text \
	--redefine-sym _binary_$*_raw_start=$* \
	--redefine-sym _binary_$*_raw_end=$*_end \
	--redefine-sym _binary_$*_raw_size=$*_size_sym \
	$(AVR_ARCHITECTURE_PARAMETER) \
	$< $@


bootloader.h: bootloader.o
#	@echo generating bootloader.h
# for internal echo command, mainly on windows
ifeq ($(OS),Windows_NT)
	@echo extern const uint8_t $* [] PROGMEM;> $@
	@echo extern const uint8_t $*_end [] PROGMEM;>> $@
	@echo extern const uint8_t $*_size_sym [];>> $@
	@echo #define $*_size ( (int) $*_size_sym )>> $@
	@echo #define $*_address 0x$(BOOTLOADER_ADDRESS)>> $@
else
# for unix echo command
	@echo "extern const uint8_t" $*"[] PROGMEM;" > $@
	@echo "extern const uint8_t" $*_end"[] PROGMEM;" >> $@
	@echo "extern const uint8_t" $*_size_sym"[];" >> $@
	@echo "#define $*_size ( (int) $*_size_sym )" >> $@
	@echo "#define $*_address 0x$(BOOTLOADER_ADDRESS)" >> $@
endif
