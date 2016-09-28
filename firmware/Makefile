# Name: Makefile
# Project: Micronucleus
# Author: Jenna Fox; portions by Christian Starkjohann, Louis Beaudoin
# Creation Date: 2007-12-10
# Tabsize: 4
# Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
# License: GNU GPL v2 (see License.txt)

#CONFIG ?= t85_default
#CONFIG ?= t85_aggressive
#CONFIG ?= t841_default
#CONFIG ?= t167_default
CONFIG ?= t84_export

###############################################################################
# Configure the following variables according to your AVR.
# Program the device with
#     make fuse    			# to set the clock generator, boot section size etc.
#     make flash   			# to load the boot loader into flash
#     make disablereset		# use external reset line for IO (CAUTION: this is not easy to enable again, see README) 

CFLAGS =
CONFIGPATH	  = configuration/$(CONFIG)
include $(CONFIGPATH)/Makefile.inc

PROGRAMMER ?= -c usbtiny
# PROGRAMMER contains AVRDUDE options to address your programmer

# Tools:
AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE) 
CC = avr-gcc

# single pass: BOOTLOADER_ADDRESS initialized to a valid address
# the bootloader starts at that address, export table not generated
#
# two pass: BOOTLOADER_ADDRESS initialized to 0
# optimal BOOTLOADER_ADDRESS calculated in pass 1, applied in pass 2
# export table not generated
#
# export table generation: BOOTLOADER_DATA initialized to 0
# BOOTLOADER_DATA calculated in pass 1, used to place global data above stack in pass 2
# BOOTLOADER_EXPORTS calculated in pass 1, used to place export table at FLASHEND in pass 2

# Options:
CFLAGS += -I. -g2 -Os # -Wall
CFLAGS += -I$(CONFIGPATH) -mmcu=$(DEVICE) -DF_CPU=$(F_CPU)
ifeq ($(BOOTLOADER_ADDRESS),0)
  # the codesize turns out wrong if we leave BOOTLOADER_ADDRESS == 0
  CFLAGS += -DBOOTLOADER_ADDRESS=0x100
else
  CFLAGS += -DBOOTLOADER_ADDRESS=0x$(BOOTLOADER_ADDRESS)
endif
CFLAGS += -nostartfiles -ffunction-sections -fdata-sections -fpack-struct -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop

LDFLAGS = -Wl,--relax,--section-start=.text=$(BOOTLOADER_ADDRESS),--gc-sections,-Map=main.map
ifdef BOOTLOADER_DATA
  # prepare global data to enable exports
  CFLAGS += -DBOOTLOADER_DATA=0x$(BOOTLOADER_DATA)
  LDFLAGS += -Wl,--undefined=__exports
  ifneq ($(BOOTLOADER_DATA),0)
    # place the data above the stack
    LDFLAGS += -Wl,--section-start=.data=0x$(shell echo "obase=16; ibase=16; scale=0; $(BOOTLOADER_DATA)+800000" | bc)
  endif
endif
ifdef BOOTLOADER_EXPORTS
  # place the exports at end of flash
  LDFLAGS += -Wl,--section-start=.exports=$(BOOTLOADER_EXPORTS)
endif

OBJECTS = usbdrv/usbdrvasm.o usbdrv/oddebug.o main.o 
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

flash:	all $(if $(SERIAL),serial.hex)
	$(AVRDUDE) -U flash:w:main.hex:i $(if $(SERIAL),-U flash:w:serial.hex:i) -B 20

.PHONY: serial.hex
.INTERMEDIATE: serial.hex
serial.hex:
	@(echo "\":02\" ; obase=16; scale=0; a=$(FLASHEND)-1; \
	a / 4096 % 16; a / 256 % 16; a / 16 % 16; a % 16; \"00\" ;\
	d=$(SERIAL); (d2 = d / 16 % 16); (d3 = d % 16); \
	(d0 = d / 4096 % 16); (d1 = d / 256 % 16); \
	(c = (8*256 - 2 - a / 256 - a % 256 - d / 256 - d % 256) % 256) / 16; \
	c % 16" | bc | tr -d '\n' ; \
	echo ; echo :00000001FF) > $@

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
ifeq ($(BOOTLOADER_ADDRESS),0)
# now that we know the size of the bootloader, we can build
# it with the correct address by second pass
	@rm main.o crt1.o ; $(MAKE) $@ BOOTLOADER_ADDRESS=$$( \
	avr-size $@ | tail -n 1 | ( read size dummy ; echo "obase=16; scale=0; \
	$(FLASHEND)+1 - ($$size + $(SPM_PAGESIZE)-1) / $(SPM_PAGESIZE) * $(SPM_PAGESIZE)") | bc) \
	$(if $(BOOTLOADER_DATA), BOOTLOADER_DATA=$$(avr-size $@ | tail -n 1 | ( read dummy0 size0 size1 dummy1 ; \
	echo "obase=16; scale=0; $(RAMEND)+1 - $$size0 - $$size1") | bc)) \
	$(if $(BOOTLOADER_DATA), BOOTLOADER_EXPORTS=$$(avr-size -A $@ | grep ^.exports | ( read section size dummy ; \
	echo "obase=16; scale=0; $(FLASHEND)+1 - $$size") | bc))
endif

main.hex:	main.bin
	@echo Building Micronucleus configuration: $(CONFIG)
	@rm -f main.hex main.eep.hex
	@avr-objcopy -j .text -j .exports -O ihex main.bin main.hex
	@echo Size of sections:
	@avr-size main.bin	
	@echo Size of binary hexfile. Use the "data" size to calculate the bootloader address:
	@avr-size main.hex
ifdef SPM_PAGESIZE
	@echo -n "BOOTLOADER_ADDRESS configured: $(BOOTLOADER_ADDRESS), calculated: " ;\
	avr-size $@ | tail -n 1 | ( read dummy0 size dummy1 ; echo "obase=16; scale=0; \
	(address = ($(FLASHEND)+1 - ($$size + $(SPM_PAGESIZE)-1) / $(SPM_PAGESIZE) * $(SPM_PAGESIZE))); \
	\"padding (decimal): \" ; obase=10; $(FLASHEND)+1 - address - $$size") | bc
endif

upgrade: main.bin
	avr-objcopy -O binary main.bin main.raw
	avr-objcopy -I binary -O elf32-avr \
        --rename-section .data=.text \
        --redefine-sym _binary_main_raw_start=loader \
        --redefine-sym _binary_main_raw_end=loader_end \
		main.raw bootloader_linkable.o	
	
disasm:	main.bin
	@avr-objdump -d -S main.bin >main.lss
