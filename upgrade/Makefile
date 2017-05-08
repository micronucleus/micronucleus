# Name: Makefile
# Project: Upgrade - based on Micronucleus makefile
# Author: Jenna Fox; portions by Christian Starkjohann, Louis Beaudoin
# Creation Date: 2012-10-4
# Tabsize: 4
# License: GNU GPL v2 (see License.txt)

###############################################################################
# Configure the following variables according to your AVR.
# Program the device with
#		 make fuse					# to set the clock generator, boot section size etc.
#		 make flash	 			# to load the boot loader into flash
#		 make lock					# to protect the boot loader from overwriting
#		 make disablereset		# for ATtiny85 target - to use external reset line for IO (CAUTION: this is not easy to enable again, see README) 

F_CPU = 16500000
DEVICE = attiny85
FUSEOPT = $(FUSEOPT_t85)
LOCKOPT = -U lock:w:0x2f:m

# app starts two pages in, so we can mess around with the first page for leet ISR hax
APP_ADDRESS = 80

PROGRAMMER = -c USBasp
# PROGRAMMER contains AVRDUDE options to address your programmer

FUSEOPT_8 = -U hfuse:w:0xc0:m -U lfuse:w:0x9f:m
FUSEOPT_88 = -U hfuse:w:0xd6:m -U lfuse:w:0xdf:m -U efuse:w:0x00:m
FUSEOPT_168 = -U hfuse:w:0xd6:m -U lfuse:w:0xdf:m -U efuse:w:0x00:m
FUSEOPT_328 = -U lfuse:w:0xf7:m -U hfuse:w:0xda:m -U efuse:w:0x03:m
FUSEOPT_t85 = -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m
FUSEOPT_t85_DISABLERESET = -U lfuse:w:0xe1:m -U efuse:w:0xfe:m -U hfuse:w:0x5d:m
# You may have to change the order of these -U commands.

#---------------------------------------------------------------------
# ATMega8
#---------------------------------------------------------------------
# Fuse high byte:
# 0xc0 = 1 1 0 0	 0 0 0 0 <-- BOOTRST (boot reset vector at 0x1800)
#				^ ^ ^ ^	 ^ ^ ^------ BOOTSZ0
#				| | | |	 | +-------- BOOTSZ1
#				| | | |	 + --------- EESAVE (preserve EEPROM over chip erase)
#				| | | +-------------- CKOPT (full output swing)
#				| | +---------------- SPIEN (allow serial programming)
#				| +------------------ WDTON (WDT not always on)
#				+-------------------- RSTDISBL (reset pin is enabled)
# Fuse low byte:
# 0x9f = 1 0 0 1	 1 1 1 1
#				^ ^ \ /	 \--+--/
#				| |	|			 +------- CKSEL 3..0 (external >8M crystal)
#				| |	+--------------- SUT 1..0 (crystal osc, BOD enabled)
#				| +------------------ BODEN (BrownOut Detector enabled)
#				+-------------------- BODLEVEL (2.7V)
#---------------------------------------------------------------------
# ATMega88, ATMega168
#---------------------------------------------------------------------
# Fuse extended byte:
# 0x00 = 0 0 0 0	 0 0 0 0 <-- BOOTRST (boot reset vector at 0x1800)
#										\+/
#										 +------- BOOTSZ (00 = 2k bytes)
# Fuse high byte:
# 0xd6 = 1 1 0 1	 0 1 1 0
#				^ ^ ^ ^	 ^ \-+-/
#				| | | |	 |	 +------ BODLEVEL 0..2 (110 = 1.8 V)
#				| | | |	 + --------- EESAVE (preserve EEPROM over chip erase)
#				| | | +-------------- WDTON (if 0: watchdog always on)
#				| | +---------------- SPIEN (allow serial programming)
#				| +------------------ DWEN (debug wire enable)
#				+-------------------- RSTDISBL (reset pin is enabled)
# Fuse low byte:
# 0xdf = 1 1 0 1	 1 1 1 1
#				^ ^ \ /	 \--+--/
#				| |	|			 +------- CKSEL 3..0 (external >8M crystal)
#				| |	+--------------- SUT 1..0 (crystal osc, BOD enabled)
#				| +------------------ CKOUT (if 0: Clock output enabled)
#				+-------------------- CKDIV8 (if 0: divide by 8)
#---------------------------------------------------------------------
# ATMega328P
#---------------------------------------------------------------------
# Fuse extended byte:
# 0x03 = - - - -	 - 0 1 1
#										\-+-/
#											+------ BODLEVEL 0..2 (011 = 4.3V)
# Fuse high byte:
# 0xda = 1 1 0 1	 1 0 1 0 <-- BOOTRST (0 = jump to bootloader at start)
#				^ ^ ^ ^	 ^ \+/
#				| | | |	 |	+------- BOOTSZ 0..1 (01 = 2KB starting at 0x7800)
#				| | | |	 + --------- EESAVE (don't preserve EEPROM over chip erase)
#				| | | +-------------- WDTON (1 = watchdog disabled at start)
#				| | +---------------- SPIEN (0 = allow serial programming)
#				| +------------------ DWEN (1 = debug wire disable)
#				+-------------------- RSTDISBL (1 = reset pin is enabled)
# Fuse low byte:
# 0xf7 = 1 1 1 1	 0 1 1 1
#				^ ^ \ /	 \--+--/
#				| |	|			 +------- CKSEL 3..0 (0111 = external full-swing crystal)
#				| |	+--------------- SUT 1..0 (11 = startup time 16K CK/14K + 65ms)
#				| +------------------ CKOUT (1 = clock output disabled)
#				+-------------------- CKDIV8 (1 = do not divide clock by 8)
#---------------------------------------------------------------------
# ATtiny85
#---------------------------------------------------------------------
# Fuse extended byte:
# 0xFE = - - - -	 - 1 1 0
#												^
#												|
#												+---- SELFPRGEN (enable self programming flash)
#
# Fuse high byte:
# 0xdd = 1 1 0 1	 1 1 0 1
#				^ ^ ^ ^	 ^ \-+-/ 
#				| | | |	 |	 +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#				| | | |	 +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#				| | | +-------------- WDTON (watchdog timer always on -> disable)
#				| | +---------------- SPIEN (enable serial programming -> enabled)
#				| +------------------ DWEN (debug wire enable)
#				+-------------------- RSTDISBL (disable external reset -> enabled)
#
# Fuse high byte ("no reset": external reset disabled, can't program through SPI anymore)
# 0x5d = 0 1 0 1	 1 1 0 1
#				^ ^ ^ ^	 ^ \-+-/ 
#				| | | |	 |	 +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#				| | | |	 +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#				| | | +-------------- WDTON (watchdog timer always on -> disable)
#				| | +---------------- SPIEN (enable serial programming -> enabled)
#				| +------------------ DWEN (debug wire enable)
#				+-------------------- RSTDISBL (disable external reset -> disabled!)
#
# Fuse low byte:
# 0xe1 = 1 1 1 0	 0 0 0 1
#				^ ^ \+/	 \--+--/
#				| |	|			 +------- CKSEL 3..0 (clock selection -> HF PLL)
#				| |	+--------------- SUT 1..0 (BOD enabled, fast rising power)
#				| +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#				+-------------------- CKDIV8 (divide clock by 8 -> don't divide)

###############################################################################

# Tools:
AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
CC = avr-gcc

# Options:
DEFINES = -DAPP_ADDRESS=0x$(APP_ADDRESS) #-DDEBUG_LEVEL=2
# Remove the -fno-* options when you use gcc 3, it does not understand them
CFLAGS = -Wall -Os -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions -I. -Ilibs-device -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) $(DEFINES)
LDFLAGS = -Wl,--relax,--gc-sections -Wl,--section-start=.text=$(APP_ADDRESS),-Map=upgrade.map

OBJECTS =	upgrade.o

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

flash:	all
	$(AVRDUDE) -U flash:w:upgrade.hex:i

readflash:
	$(AVRDUDE) -U flash:r:read.hex:i

fuse:
	$(AVRDUDE) $(FUSEOPT)

disablereset:
	$(AVRDUDE) $(FUSEOPT_t85_DISABLERESET)

lock:
	$(AVRDUDE) $(LOCKOPT)

read_fuses:
	$(UISP) --rd_fuses

clean:
	rm -f upgrade.hex upgrade.bin *.o upgrade.s upgrade.map upgrade.c.lst

# file targets:
upgrade.bin:	$(OBJECTS)
	$(CC) $(CFLAGS) -o upgrade.bin $(OBJECTS) $(LDFLAGS)

upgrade.hex:	upgrade.bin
	rm -f upgrade.hex upgrade.eep.hex
	avr-objcopy -j .text -j .data -O ihex upgrade.bin upgrade-app.hex
	cat upgrade-prefix.hex upgrade-app.hex > upgrade.hex
	rm upgrade-app.hex
	avr-size upgrade.hex

disasm:	upgrade.bin
	avr-objdump -d upgrade.bin

cpp:
	$(CC) $(CFLAGS) -E upgrade.c


