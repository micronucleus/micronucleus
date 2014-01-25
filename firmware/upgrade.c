// Upgrade is an in-place firmware upgrader for tiny85 chips - just fill in the
// 'bootloaderAddress' variable in bootloader_data.h, and the bootloaderData
// progmem array with the bootloader data, and you're ready to go.
// 
// Upgrade will firstly rewrite the interrupt vector table to disable the bootloader,
// rewriting it to just run the upgrade app. Next it erases and writes each page of the
// bootloader in sequence, erasing over any remaining pages leaving them set to 0xFFFF
// Finally upgrader erases it's interrupt table again and fills it with RJMPs to
// bootloaderAddress, effectively bridging the interrupts in to the new bootloader's
// interrupts table.
//
// While upgrade has been written with attiny85 and micronucleus in mind, it should
// work with other bootloaders and other chips with flash self program but no hardware
// bootloader protection, where the bootloader exists at the end of flash
//
// Be very careful to not power down the AVR while upgrader is running.
// If you connect a piezo between pb0 and pb1 you'll hear a bleep when the update
// is complete. You can also connect an LED with pb0 positive and pb1 or gnd negative and
// it will blink

#include "./utils.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include "./bootloader_data.c"

void secure_interrupt_vector_table(void);
void write_new_bootloader(void);
void forward_interrupt_vector_table(void);
void beep(void);
void reboot(void);

void load_table(uint16_t address, uint16_t words[SPM_PAGESIZE / 2]);
void erase_page(uint16_t address);
void write_page(uint16_t address, uint16_t words[SPM_PAGESIZE / 2]);


int main(void) {
  pinsOff(0xFF); // pull down all pins
  outputs(0xFF); // all to ground - force usb disconnect
  delay(250); // milliseconds
  inputs(0xFF); // let them float
  delay(250);
  cli();
  
  secure_interrupt_vector_table(); // reset our vector table to it's original state
  write_new_bootloader();
  forward_interrupt_vector_table();
  
  beep();
  
  reboot();
  
  return 0;
}

// erase first page, removing any interrupt table hooks the bootloader added when
// upgrade was uploaded
void secure_interrupt_vector_table(void) {
  uint16_t table[SPM_PAGESIZE / 2];
  
  load_table(0, table);
  
  // wipe out any interrupt hooks the bootloader rewrote
  int i = 0;
  while (i < SPM_PAGESIZE / 2) {
    table[0] = 0xFFFF;
    i++;
  }
  
  erase_page(0);
  write_page(0, table);
}

// erase bootloader's section and write over it with new bootloader code
void write_new_bootloader(void) {
  uint16_t outgoing_page[SPM_PAGESIZE / 2];
  int iter = 0;
  while (iter < sizeof(bootloader_data)) {
    
    // read in one page's worth of data from progmem
    int word_addr = 0;
    while (word_addr < SPM_PAGESIZE) {
      int subaddress = ((int) bootloader_data) + iter + word_addr;
      if (subaddress >= ((int) bootloader_data) + sizeof(bootloader_data)) {
        outgoing_page[word_addr / 2] = 0xFFFF;
      } else {
        outgoing_page[word_addr / 2] = pgm_read_word(subaddress);
      }
      
      word_addr += 2;
    }
    
    // erase page in destination
    erase_page(bootloader_address + iter);
    // write updated page
    write_page(bootloader_address + iter, outgoing_page);
    
    iter += 64;
  }
}

// write in forwarding interrupt vector table
void forward_interrupt_vector_table(void) {
  uint16_t vector_table[SPM_PAGESIZE / 2];
  
  int iter = 0;
  while (iter < SPM_PAGESIZE / 2) {
    // rjmp to bootloader_address's interrupt table
    vector_table[iter] = 0xC000 + (bootloader_address / 2) - 1;
    iter++;
  }
  
  erase_page(0);
  write_page(0, vector_table);
}

void load_table(uint16_t address, uint16_t words[SPM_PAGESIZE / 2]) {
  uint16_t subaddress = 0;
  address -= address % SPM_PAGESIZE; // round down to nearest page start
  
  while (subaddress < SPM_PAGESIZE) {
    words[subaddress / 2] = pgm_read_word(address + subaddress);
    subaddress += 2;
  }
}

void erase_page(uint16_t address) {
  boot_page_erase(address - (address % SPM_PAGESIZE));
  boot_spm_busy_wait();
}

void write_page(uint16_t address, uint16_t words[SPM_PAGESIZE / 2]) {
  // fill buffer
  uint16_t iter = 0;
  while (iter < SPM_PAGESIZE / 2) {
    boot_page_fill(address + (iter * 2), words[iter]);
    iter++;
  }
  
  boot_page_write(address);
  boot_spm_busy_wait(); // Wait until the memory is written.
}

// beep for a quarter of a second
void beep(void) {
  outputs(pin(0) | pin(1));
  pinOff(1);
  
  byte i = 0;
  while (i < 250) {
    delay(1);
    pinOn(pin(0));
    delay(1);
    pinOff(pin(0));
    i++;
  }
}

void reboot(void) {
  void (*ptrToFunction)(); // pointer to a function 
  ptrToFunction = 0x0000;
  (*ptrToFunction)(); // reset!
}


////////////// Add padding to start of program so no program code could reasonably be erased while program is running
// this never needs to be called - avr-gcc stuff happening: http://www.nongnu.org/avr-libc/user-manual/mem_sections.html
volatile void FakeISR (void) __attribute__ ((naked)) __attribute__ ((section (".init0")));
volatile void FakeISR (void) {
  // 16 nops to pad out first section of program
  asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
  asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
}
