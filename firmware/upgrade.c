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
//

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>

#include "chipid.h"
#include <hardware.h>
#include "portnames.h"

#define TINY_TABLE_LEN (VECTOR_SIZE * 2 + 2)

#define PAGE_WORDS  (SPM_PAGESIZE / 2)


/*
 * Note: __payload_{start,end} are symbols provided by the linker. As such
 *       they are just addresses and do not have a size.
 *
 * Note: The compiler does not regard any address calculations with
 *       these addreses as constant, so we cannot store the bootloader size
 *       in a global variable, which would require a static initializer.
 *
 * Note: the type we assign here does matter. Taking the address of a function
 *       returns a word number, addresses to data items are in byte numbers
 *       instead.  Thus, by declaring those addreses as "const PROGMEM char",
 *       we will get the size in bytes later.
 */
extern void __ctors_end(void);
extern const PROGMEM char __payload_start;
extern const PROGMEM char __payload_end;
extern const PROGMEM char __payload_vectors_start;
extern const PROGMEM char __payload_vectors_end;

uint16_t *bootloader_data = (uint16_t *)&__payload_start;
uint16_t *vector_data = (uint16_t *)&__payload_vectors_start;

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
	uint16_t vector_words = (&__payload_vectors_end - &__payload_vectors_start) / 2;

	load_table(0, vector_table);

	vector_table[0] = addr2rjmp( (uint16_t)&(__ctors_end), 0);
	vector_table[1] = vector_words;

	erase_page(0);
	write_page(0, vector_table);
}

/*
 * write in forwarding interrupt vector table.
 *
 */
static void forward_interrupt_vector_table(void)
{
	uint16_t page_buffer[PAGE_WORDS];
	uint16_t vector_words = (&__payload_vectors_end - &__payload_vectors_start) / 2;

	memset(page_buffer, 0xFF, sizeof(page_buffer));

	if ( vector_words < 1 ) {

		/* modify reset vector */
		page_buffer[0] = addr2rjmp(((BOOTLOADER_ADDRESS + TINY_TABLE_LEN)/ 2), 0);
		page_buffer[USB_INTR_VECTOR_NUM * (VECTOR_SIZE / 2)] =
			addr2rjmp(((BOOTLOADER_ADDRESS + TINY_TABLE_LEN + 2)/ 2), USB_INTR_VECTOR_NUM * (VECTOR_SIZE/2));

		erase_page(0);
		write_page(0, page_buffer);
	} else {
		/*
		 * FIXME: this is essentially replicated code. Would be nice
		 * to have a flashcpy() function, memcpy() style.
		 */
		uint16_t write_addr = 0;
		uint16_t offset = 0;
		uint8_t  page_word = 0;

		for (offset = 0; offset < vector_words ; offset ++) {
			uint16_t data = pgm_read_word( vector_data + offset );

			page_buffer[ page_word ] = data;
			write_addr += 2;
			page_word ++;

			if ( (write_addr > 0) && (write_addr % SPM_PAGESIZE) == 0) {
				erase_page(write_addr - SPM_PAGESIZE);
				write_page(write_addr - SPM_PAGESIZE, page_buffer);
				page_word = 0;
			}
		}

		if ( (write_addr % SPM_PAGESIZE) != 0) {
			while ( (write_addr % SPM_PAGESIZE) != 0 ) {
				page_buffer[ page_word ] = 0xFFFF;
				write_addr += 2;
				page_word ++;
			}

			erase_page(write_addr - SPM_PAGESIZE);
			write_page(write_addr - SPM_PAGESIZE, page_buffer);
		}
	}
}

// erase bootloader's section and write over it with new bootloader code
static void write_new_bootloader(void)
{

	uint16_t page_buffer[PAGE_WORDS];
	uint8_t  page_word = 0;
	uint16_t write_addr = BOOTLOADER_ADDRESS - (BOOTLOADER_ADDRESS % SPM_PAGESIZE);
	uint16_t offset = 0;
	uint16_t bootloader_words = (&__payload_end - &__payload_start) / 2;

	while (write_addr < BOOTLOADER_ADDRESS) {
		page_buffer[ page_word ] = 0xFFFF;
		write_addr += 2;
		page_word ++;
	}

	for (offset = 0; offset < bootloader_words ; offset ++) {
		uint16_t data = pgm_read_word( bootloader_data + offset );

		page_buffer[ page_word ] = data;
		write_addr += 2;
		page_word ++;

		if ( (write_addr % SPM_PAGESIZE) == 0) {

			erase_page(write_addr - SPM_PAGESIZE);
			write_page(write_addr - SPM_PAGESIZE, page_buffer);
			page_word = 0;
		}
	}

	if ( (write_addr % SPM_PAGESIZE) != 0) {
		while ( (write_addr % SPM_PAGESIZE) != 0 ) {
			page_buffer[ page_word ] = 0xFFFF;
			write_addr += 2;
			page_word ++;
		}

		erase_page(write_addr - SPM_PAGESIZE);
		write_page(write_addr - SPM_PAGESIZE, page_buffer);
	}
}

static inline void usb_disconnect(void)
{
	PORT_PORT(USB_CFG_IOPORTNAME) &= ~_BV(USB_CFG_DMINUS_BIT);
	PORT_DDR(USB_CFG_IOPORTNAME)  |= _BV(USB_CFG_DMINUS_BIT);
}

static inline void reboot(void) __attribute__((__noreturn__));
static inline void reboot(void)
{
	asm volatile ( "rjmp __vectors" );
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5))
	__builtin_unreachable();
#endif
}

int main(void) __attribute__((__noreturn__));
int main(void)
{
	cli();

	/*
	 * Pull down D- (= disconnect USB) so the host does not try to
	 * enumerate
	 */
	usb_disconnect();

	secure_interrupt_vector_table(); // reset our vector table to it's original state
	write_new_bootloader();
	forward_interrupt_vector_table();

	/*
	 * do not let the D- float, it would just cause the host to start a
	 * re-enumeration long before we have entered the bootloader, which
	 * will then disconnect again
	 */

	reboot();
}

