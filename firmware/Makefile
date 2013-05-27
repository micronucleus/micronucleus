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

DEFINES :=

include $(SRCDIR)/hardware/$(HARDWARE).mk

# PROGRAMMER = -c USBasp
PROGRAMMER = -c stk500 -P /dev/ttyACM0
# PROGRAMMER contains AVRDUDE options to address your programmer

# Tools:
AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
CC = avr-gcc

# Remove the -fno-* options when you use gcc 3, it does not understand them
CFLAGS = \
	-Wall \
	-Os \
	-g \
	-fno-move-loop-invariants \
	-fno-tree-scev-cprop \
	-fno-inline-small-functions \
	-mbranch-cost=5 \
	-I $(HW_INCLUDE_DIR) \
	-I $(SRCDIR) \
	-I $(SRCDIR)/libs-device \
	-mmcu=$(DEVICE) \
	-DF_CPU=$(F_CPU) \
	-DBOOTLOADER_ADDRESS=0x$(BOOTLOADER_ADDRESS) \
	$(DEFINES)

LDFLAGS = \
	-Wl,--relax \
	-Wl,--gc-sections

BOOTLOADER_SOURCES =  usbdrv/usbdrvasm.S bootloader.c bootloader-vectors.S

BOOTLOADER_OBJECTS =  usbdrv/usbdrvasm.o bootloader.o bootloader-vectors.o

BOOTLOADER_LDFLAGS= \
	-nostartfiles \
	-Wl,-Map=bootloader.map \
	-T$(SRCDIR)/bootloader$(LDSCRIPT_SUFFIX) \
	-Wl,--section-start=.text=$(BOOTLOADER_ADDRESS) \
	-Wl,--section-start=.vectorpage=0 \
	-Wl,--pmem-wrap-around=$(PMEM_WRAP) \

UPGRADE_OBJECTS = \
	upgrade-with-bootloader.o

UPGRADE_LDFLAGS= \
	-Wl,-Map=upgrade.map \
	-T$(SRCDIR)/upgrade$(LDSCRIPT_SUFFIX)

# define as empty to get verbose output
Q=@

VPATH=..

# symbolic targets:
all: bootloader.hex upgrade.hex

bootloader.dep:	$(BOOTLOADER_SOURCES)
	@echo GEN $@
	$(Q)$(CC) -M $(CFLAGS) $^ > $@

-include bootloader.dep

.c.o:
	$(Q)echo "CC  $< ..."
	$(Q)$(CC) $(CFLAGS) -c $< -o $@ -Wa,-ahls=$(notdir $<).lst

.S.o:
	$(Q)echo "ASM $< ..."
	$(Q)$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(CC) $(CFLAGS) -S $< -o $@

%.hex : %.elf
	$(Q)echo "HEX $@"
	$(Q)avr-objcopy -j .text -j .data -O ihex $< $@

#--------- bootloader specific -------

bootloader.raw : bootloader.elf
	$(Q)echo "GEN $@"
	$(Q)avr-objcopy -j .text -j .data -O binary $< $@

bootloader.elf:	$(BOOTLOADER_OBJECTS) $(SRCDIR)/bootloader$(LDSCRIPT_SUFFIX)
	$(Q)echo "LD  $@"
	$(Q)$(CC) $(CFLAGS) -nostartfiles -o $@ $(BOOTLOADER_OBJECTS) $(LDFLAGS) $(BOOTLOADER_LDFLAGS)

bootloader.hex : bootloader.elf
	$(Q)echo "HEX $@"
	$(Q)avr-objcopy -j .vectorpage -j .text -j .data -O ihex $< $@

flash:	bootloader.hex
	$(AVRDUDE) -U flash:w:bootloader.hex:i

disasm:	bootloader.elf
	avr-objdump -S -d bootloader.elf


#--------- upgrade specific -------

UPGRADEABLE_FIRMWARE=bootloader.elf

upgrade-trampolines.sym: $(UPGRADEABLE_FIRMWARE)
	@echo GEN $@
	$(Q)$(SRCDIR)/syms-for-bootloader.sh $< >$@

upgrade-trampolines.elf:	upgrade-trampolines.o upgrade-trampolines.sym
	@echo "LD  $@"
	$(Q)$(CC) $(CFLAGS) \
		-Wl,--just-symbols=upgrade-trampolines.sym\
		-Wl,--pmem-wrap-around=$(PMEM_WRAP) \
		-Wl,--relax  \
		-nostartfiles -o $@ $<

upgrade-trampolines.raw:	upgrade-trampolines.elf
	@echo "GEN $@"
	$(Q)avr-objcopy -j .text -O binary $< $@

upgrade-with-bootloader.o:	upgrade.o upgrade-trampolines.raw bootloader.raw
	@echo "GEN $@"
	$(Q)avr-objcopy \
		--add-section .payloadvectors=upgrade-trampolines.raw \
		--set-section-flags .payloadvectors=alloc \
		--add-section .payload=bootloader.raw \
		--set-section-flags .payload=alloc \
		-O elf32-avr $< $@

upgrade.elf:	$(UPGRADE_OBJECTS) $(SRCDIR)/upgrade$(LDSCRIPT_SUFFIX)
	@echo "LD  $@"
	$(Q)$(CC) $(CFLAGS) -o $@ $(UPGRADE_OBJECTS) $(LDFLAGS) $(UPGRADE_LDFLAGS)

upgrade.hex:    upgrade.elf
	@echo "GEN $@"
	$(Q)avr-objcopy -j .text -j .data -j .payload -j .payloadvectors -O ihex $< $@

disasm-upgrade:	upgrade.elf
	avr-objdump -S -d upgrade.elf

flash-upgrade:	upgrade.hex
	$(AVRDUDE) -U flash:w:upgrade.hex:i

#--------- other targets -------

readflash:
	$(AVRDUDE) -U flash:r:read.hex:i

fuse:
	$(AVRDUDE) $(FUSEOPT)

disablereset:
	$(AVRDUDE) $(FUSEOPT_t85_DISABLERESET)

clean:
	rm -f bootloader.hex bootloader.elf bootloader.c.lst bootloader.map *.o usbdrv/*.o bootloader.s usbdrv/oddebug.s usbdrv/usbdrv.s libs-device/osccal.o

cpp:
	$(CC) $(CFLAGS) -E bootloader.c

endif
