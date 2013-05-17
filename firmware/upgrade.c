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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>

#define PAGE_WORDS  (SPM_PAGESIZE / 2)

/*
 * Note: __payload_{start,end} are symbols provided by the linker. As such
 *       they are just addresses and do not have a size.
 *
 * Note: The compiler does not regard any address calculations with
 *       these addreses as constant, so we cannot set the bootloader size
 *       through a static initializer - we do it later in "main()".
 *
 * Note: the type we assign here does matter. Taking the address of a function
 *       returns a word number, addresses to data items are in byte numbers
 *       instead.  Thus, when we later calculate the bootloader size, we will
 *       get that size in bytes.
 */
extern void __ctors_end(void);
extern const PROGMEM char __payload_start;
extern const PROGMEM char __payload_end;

uint16_t *bootloader_data = (uint16_t *)&__payload_start;

uint16_t bootloader_size = 0;

static void erase_page(uint16_t address)
{
	boot_page_erase(address - (address % SPM_PAGESIZE));
	boot_spm_busy_wait();
}

static void write_page(uint16_t address, uint16_t words[PAGE_WORDS])
{
	// fill buffer
	uint16_t iter = 0;

	while (iter < PAGE_WORDS) {
		boot_page_fill(address + (iter * 2), words[iter]);
		iter++;
	}

	boot_page_write(address);
	boot_spm_busy_wait(); // Wait until the memory is written.
}

static void load_table(uint16_t address, uint16_t words[PAGE_WORDS])
{
	uint16_t subaddress = 0;

	address -= address % SPM_PAGESIZE; // round down to nearest page start

	while (subaddress < SPM_PAGESIZE) {
		words[subaddress / 2] = pgm_read_word(address + subaddress);
		subaddress += 2;
	}
}

#define addr2rjmp(addr, location) \
	( 0xC000 + ((addr - location - 1) & 0xFFF))

// erase first page, removing any interrupt table hooks the bootloader added when
// upgrade was uploaded
static void secure_interrupt_vector_table(void)
{
	uint16_t vector_table[PAGE_WORDS];

	load_table(0, vector_table);

	vector_table[0] = addr2rjmp( (uint16_t)&(__ctors_end), 0);

	erase_page(0);
	write_page(0, vector_table);
}

// write in forwarding interrupt vector table
static void forward_interrupt_vector_table(void)
{
	uint16_t vector_table[PAGE_WORDS];

	// int iter = 0;

	load_table(0, vector_table);

	/* modify reset vector */
	vector_table[0] = addr2rjmp((BOOTLOADER_ADDRESS / 2), 0);

	erase_page(0);
	write_page(0, vector_table);
}


// erase bootloader's section and write over it with new bootloader code
static void write_new_bootloader(void)
{

	uint16_t write_addr = BOOTLOADER_ADDRESS - (BOOTLOADER_ADDRESS % SPM_PAGESIZE);
	uint16_t offset = 0;

	while (write_addr < BOOTLOADER_ADDRESS) {
		boot_page_fill_safe(write_addr, 0xFFFF);
		write_addr += 2;
	}

	for (offset = 0; offset < (bootloader_size / 2); offset ++) {
		uint16_t data = pgm_read_word( bootloader_data + offset );
		boot_page_fill_safe(write_addr, data);
		write_addr += 2;

		if ( write_addr % SPM_PAGESIZE == 0) {
			boot_page_erase_safe(write_addr - 1);
			boot_page_write_safe(write_addr - 1);
		}
	}

	if ( write_addr % SPM_PAGESIZE != 0) {
		while ( (write_addr % SPM_PAGESIZE) != 0 ) {
			boot_page_fill_safe(write_addr, 0xFFFF);
			write_addr += 2;
		}

		boot_page_erase_safe(write_addr - 1);
		boot_page_write_safe(write_addr - 1);
	}
}

static void reboot(void)
{
	asm volatile ( "rjmp 0" );
}

int main(void)
{
	cli();
	bootloader_size = &__payload_end - &__payload_start;

	// pinsOff(0xFF);  // pull down all pins
	// outputs(0xFF);  // all to ground - force usb disconnect
	// delay(250);     // milliseconds
	// inputs(0xFF);   // let them float
	// delay(250);

	secure_interrupt_vector_table(); // reset our vector table to it's original state
	write_new_bootloader();
	forward_interrupt_vector_table();

	reboot();

	return 0;
}
