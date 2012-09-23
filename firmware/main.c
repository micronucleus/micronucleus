/* Name: main.c
 * Project: USBaspLoader
 * Author: Christian Starkjohann
 * Creation Date: 2007-12-08
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * Portions Copyright: (c) 2012 Louis Beaudoin
 * License: GNU GPL v2 (see License.txt)
 * This Revision: $Id: main.c 786 2010-05-30 20:41:40Z cs $
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>

static void leaveBootloader() __attribute__((__noreturn__));

#include "bootloaderconfig.h"
#include "usbdrv/usbdrv.c"

/* ------------------------------------------------------------------------ */

/* Request constants used by USBasp */
#define USBASP_FUNC_CONNECT         1
#define USBASP_FUNC_DISCONNECT      2
#define USBASP_FUNC_TRANSMIT        3
#define USBASP_FUNC_READFLASH       4
#define USBASP_FUNC_ENABLEPROG      5
#define USBASP_FUNC_WRITEFLASH      6
#define USBASP_FUNC_READEEPROM      7
#define USBASP_FUNC_WRITEEEPROM     8
#define USBASP_FUNC_SETLONGADDRESS  9

/* ------------------------------------------------------------------------ */

#ifndef ulong
#   define ulong    unsigned long
#endif
#ifndef uint
#   define uint     unsigned int
#endif

/* defaults if not in config file: */
#ifndef HAVE_EEPROM_PAGED_ACCESS
#   define HAVE_EEPROM_PAGED_ACCESS 0
#endif
#ifndef HAVE_EEPROM_BYTE_ACCESS
#   define HAVE_EEPROM_BYTE_ACCESS  0
#endif
#ifndef BOOTLOADER_CAN_EXIT
#   define  BOOTLOADER_CAN_EXIT     0
#endif

/* allow compatibility with avrusbboot's bootloaderconfig.h: */
#ifdef BOOTLOADER_INIT
#   define bootLoaderInit()         BOOTLOADER_INIT
#   define bootLoaderExit()
#endif
#ifdef BOOTLOADER_CONDITION
#   define bootLoaderCondition()    BOOTLOADER_CONDITION
#endif

/* device compatibility: */
#ifndef GICR    /* ATMega*8 don't have GICR, use MCUCR instead */
#   define GICR     MCUCR
#endif

/* ------------------------------------------------------------------------ */

#if (FLASHEND) > 0xffff /* we need long addressing */
#   define CURRENT_ADDRESS  currentAddress.l
#   define addr_t           ulong
#else
#   define CURRENT_ADDRESS  currentAddress.w[0]
#   define addr_t           uint
#endif

typedef union longConverter{
    addr_t  l;
    uint    w[sizeof(addr_t)/2];
    uchar   b[sizeof(addr_t)];
}longConverter_t;

#ifdef TINY85MODE
static uchar            flashPageLoaded = 0;
#if HAVE_CHIP_ERASE
static uchar            eraseRequested = 0;
#endif
static uchar            appWriteComplete = 0;
static uint16_t         writeSize;
static uint16_t         vectorTemp[2];
static uchar 			needToErase = 0;
#endif

#ifdef APPCHECKSUM
static uchar            connectedToPc = 0;
static uint16_t         checksum = 0;
#endif

static uchar            requestBootLoaderExit;
static longConverter_t  currentAddress; /* in bytes */
static uchar            bytesRemaining;
static uchar            isLastPage;
#if HAVE_EEPROM_PAGED_ACCESS
static uchar            currentRequest;
#else
static const uchar      currentRequest = 0;
#endif

static const uchar  signatureBytes[4] = {
#ifdef SIGNATURE_BYTES
    SIGNATURE_BYTES
#elif defined (__AVR_ATmega8__) || defined (__AVR_ATmega8HVA__)
    0x1e, 0x93, 0x07, 0
#elif defined (__AVR_ATmega48__) || defined (__AVR_ATmega48P__)
    0x1e, 0x92, 0x05, 0
#elif defined (__AVR_ATmega88__) || defined (__AVR_ATmega88P__)
    0x1e, 0x93, 0x0a, 0
#elif defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__)
    0x1e, 0x94, 0x06, 0
#elif defined (__AVR_ATmega328P__)
    0x1e, 0x95, 0x0f, 0
#else
#   error "Device signature is not known, please edit main.c!"
#endif
};

/* ------------------------------------------------------------------------ */

#ifdef TINY85MODE
static void writeFlashPage(void)
{
    if(needToErase)
    {
        boot_page_erase(CURRENT_ADDRESS - 2);
        boot_spm_busy_wait();
    }

    boot_page_write(CURRENT_ADDRESS - 2);
    boot_spm_busy_wait(); // Wait until the memory is written.

    needToErase = 0;
}

#define __boot_page_fill_clear()   \
(__extension__({                                 \
    __asm__ __volatile__                         \
    (                                            \
        "sts %0, %1\n\t"                         \
        "spm\n\t"                                \
        :                                        \
        : "i" (_SFR_MEM_ADDR(__SPM_REG)),        \
          "r" ((uint8_t)(__BOOT_PAGE_FILL | (1 << CTPB)))     \
    );                                           \
}))

static void writeWordToPageBuffer(uint16_t data)
{
    // first two interrupt vectors get replaced with a jump to the bootloader vector table
    if(CURRENT_ADDRESS == (RESET_VECTOR_OFFSET * 2) || CURRENT_ADDRESS == (USBPLUS_VECTOR_OFFSET * 2))
        data = 0xC000 + (BOOTLOADER_ADDRESS/2) - 1;

    // write 2's complement of checksum
#ifdef APPCHECKSUM
    if(CURRENT_ADDRESS == BOOTLOADER_ADDRESS - APPCHECKSUM_POSITION)
        data = (uint8_t)(~checksum + 1);
#endif

    if(CURRENT_ADDRESS == BOOTLOADER_ADDRESS - TINYVECTOR_RESET_OFFSET)
        data = vectorTemp[0] + ((FLASHEND + 1) - BOOTLOADER_ADDRESS)/2 + 2 + RESET_VECTOR_OFFSET;

    if(CURRENT_ADDRESS == BOOTLOADER_ADDRESS - TINYVECTOR_USBPLUS_OFFSET)
        data = vectorTemp[1] + ((FLASHEND + 1) - BOOTLOADER_ADDRESS)/2 + 1 + USBPLUS_VECTOR_OFFSET;

#ifdef APPCHECKSUM
    // only calculate checksum when we are writing new data to flash (ignore when writing vectors after bootloader programming)
    checksum += (uint8_t)(data/256) + (uint8_t)(data);
#endif

    // clear page buffer as a precaution before filling the buffer on the first page
    // TODO: maybe clear on the first byte of every page?
    if(CURRENT_ADDRESS == 0x0000)
        __boot_page_fill_clear();
    
    cli();
    boot_page_fill(CURRENT_ADDRESS, data);
    sei();

	// only need to erase if there is data already in the page that doesn't match what we're programming    
	if(pgm_read_word(CURRENT_ADDRESS) != data && pgm_read_word(CURRENT_ADDRESS) != 0xFFFF)
        needToErase = 1;

    CURRENT_ADDRESS += 2;
}

static void fillFlashWithVectors(void)
{
    int16_t i;

    // fill all or remainder of page starting at CURRENT_ADDRESS with 0xFFs, unless we're
    //   at a special address that needs a vector replaced
    for (i = CURRENT_ADDRESS % SPM_PAGESIZE; i < SPM_PAGESIZE; i += 2)
    {
        writeWordToPageBuffer(0xFFFF);
    }

    writeFlashPage();
}

#	if HAVE_CHIP_ERASE
static void eraseApplication(void)
{
    // erase all pages starting from end of application section down to page 1 (leaving page 0)
    CURRENT_ADDRESS = BOOTLOADER_ADDRESS - SPM_PAGESIZE;
    while(CURRENT_ADDRESS != 0x0000)
    {
        boot_page_erase(CURRENT_ADDRESS);
        boot_spm_busy_wait();

        CURRENT_ADDRESS -= SPM_PAGESIZE;
    }

    // erase and load page 0 with vectors
    fillFlashWithVectors();
}
#	endif
#endif

/* ------------------------------------------------------------------------ */

#ifdef APPCHECKSUM
// sum all bytes from 0x0000 through the previously saved checksum, should equal 0 for valid app
static inline int testForValidApplication(void)
{
    uint16_t i;
    uint8_t checksum = 0;
    for(i=0; i<BOOTLOADER_ADDRESS - 5; i++)
    {
        checksum += pgm_read_byte(i);
    }

    if(checksum == 0x00)
        return 1;
    else
    return 0;
}
#endif

/* ------------------------------------------------------------------------ */

#ifndef TINY85MODE
static void (*nullVector)(void) __attribute__((__noreturn__));
#endif

static inline __attribute__((noreturn)) void leaveBootloader(void)
{
    DBG1(0x01, 0, 0);
    bootLoaderExit();
    cli();
    USB_INTR_ENABLE = 0;
    USB_INTR_CFG = 0;       /* also reset config bits */

#ifdef TINY85MODE
	// make sure remainder of flash is erased and write checksum and application reset vectors
    if(appWriteComplete)
    {
        CURRENT_ADDRESS = writeSize;
        while(CURRENT_ADDRESS < BOOTLOADER_ADDRESS)
        {
            fillFlashWithVectors();
        }
    }
#else
    GICR = (1 << IVCE);     /* enable change of interrupt vectors */
    GICR = (0 << IVSEL);    /* move interrupts to application flash section */
#endif

    // TODO: watchdog reset instead to reset registers?
#ifdef APPCHECKSUM
    if(!testForValidApplication())
        asm volatile ("rjmp __vectors");
#endif

#ifdef TINY85MODE
    // clear magic word from bottom of stack before jumping to the app
    *(uint8_t*)(RAMEND) = 0x00;
    *(uint8_t*)(RAMEND-1) = 0x00;

    // jump to application reset vector at end of flash
    asm volatile ("rjmp __vectors - 4");
#else
/* We must go through a global function pointer variable instead of writing
 *  ((void (*)(void))0)();
 * because the compiler optimizes a constant 0 to "rcall 0" which is not
 * handled correctly by the assembler.
 */
    nullVector();
#endif
}

/* ------------------------------------------------------------------------ */

uchar   usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;
uchar           len = 0;
static uchar    replyBuffer[4];

#ifdef APPCHECKSUM
    connectedToPc = 1;
#endif

    usbMsgPtr = replyBuffer;
    if(rq->bRequest == USBASP_FUNC_TRANSMIT){   /* emulate parts of ISP protocol */
        uchar rval = 0;
        usbWord_t address;
        address.bytes[1] = rq->wValue.bytes[1];
        address.bytes[0] = rq->wIndex.bytes[0];
        if(rq->wValue.bytes[0] == 0x30){        /* read signature */
            rval = rq->wIndex.bytes[0] & 3;
            rval = signatureBytes[rval];
#if HAVE_EEPROM_BYTE_ACCESS
        }else if(rq->wValue.bytes[0] == 0xa0){  /* read EEPROM byte */
            rval = eeprom_read_byte((void *)address.word);
        }else if(rq->wValue.bytes[0] == 0xc0){  /* write EEPROM byte */
            eeprom_write_byte((void *)address.word, rq->wIndex.bytes[1]);
#endif
#if HAVE_CHIP_ERASE
        }else if(rq->wValue.bytes[0] == 0xac && rq->wValue.bytes[1] == 0x80){  /* chip erase */
#	ifdef TINY85MODE
            eraseRequested = 1;
#	else
            addr_t addr;
            for(addr = 0; addr < FLASHEND + 1 - 2048; addr += SPM_PAGESIZE) {
                /* wait and erase page */
                DBG1(0x33, 0, 0);
#   	ifndef NO_FLASH_WRITE
                boot_spm_busy_wait();
                cli();
                boot_page_erase(addr);
                sei();
#   	endif
            }
#	endif
#endif
        }else{
            /* ignore all others, return default value == 0 */
        }
        replyBuffer[3] = rval;
        len = 4;
    }else if(rq->bRequest == USBASP_FUNC_ENABLEPROG){
        /* replyBuffer[0] = 0; is never touched and thus always 0 which means success */
        len = 1;
    }else if(rq->bRequest >= USBASP_FUNC_READFLASH && rq->bRequest <= USBASP_FUNC_SETLONGADDRESS){
        currentAddress.w[0] = rq->wValue.word;
        if(rq->bRequest == USBASP_FUNC_SETLONGADDRESS){
#if (FLASHEND) > 0xffff
            currentAddress.w[1] = rq->wIndex.word;
#endif
        }else{
            bytesRemaining = rq->wLength.bytes[0];
            /* if(rq->bRequest == USBASP_FUNC_WRITEFLASH) only evaluated during writeFlash anyway */
            isLastPage = rq->wIndex.bytes[1] & 0x02;
#if HAVE_EEPROM_PAGED_ACCESS
            currentRequest = rq->bRequest;
#endif
            len = 0xff; /* hand over to usbFunctionRead() / usbFunctionWrite() */
        }
#if BOOTLOADER_CAN_EXIT
    }else if(rq->bRequest == USBASP_FUNC_DISCONNECT){
        requestBootLoaderExit = 1;      /* allow proper shutdown/close of connection */
#endif
    }else{
        /* ignore: USBASP_FUNC_CONNECT */
    }
    return len;
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
    uchar   isLast;

    DBG1(0x31, (void *)&currentAddress.l, 4);
    if(len > bytesRemaining)
        len = bytesRemaining;
    bytesRemaining -= len;
    isLast = bytesRemaining == 0;
#if HAVE_EEPROM_PAGED_ACCESS
    if(currentRequest >= USBASP_FUNC_READEEPROM){
        uchar i;
        for(i = 0; i < len; i++){
            eeprom_write_byte((void *)(currentAddress.w[0]++), *data++);
        }
    }else {
#endif
        uchar i;
        for(i = 0; i < len;){
#ifdef TINY85MODE
#if 1
            if(CURRENT_ADDRESS == RESET_VECTOR_OFFSET * 2)
            {
                vectorTemp[0] = *(short *)data;
            }
            if(CURRENT_ADDRESS == USBPLUS_VECTOR_OFFSET * 2)
            {
                vectorTemp[1] = *(short *)data;
            }
#else
            if(CURRENT_ADDRESS == RESET_VECTOR_OFFSET * 2 || CURRENT_ADDRESS == USBPLUS_VECTOR_OFFSET * 2)
            {
                vectorTemp[CURRENT_ADDRESS ? 1:0] = *(short *)data;
            }
#endif
#else
#	if !HAVE_CHIP_ERASE
            if((currentAddress.w[0] & (SPM_PAGESIZE - 1)) == 0){    /* if page start: erase */
                DBG1(0x33, 0, 0);
#   	ifndef NO_FLASH_WRITE
                cli();
                boot_page_erase(CURRENT_ADDRESS);   /* erase page */
                sei();
                boot_spm_busy_wait();               /* wait until page is erased */
#   	endif
#	endif
#endif

            i += 2;
            DBG1(0x32, 0, 0);
#ifdef TINY85MODE
            if(CURRENT_ADDRESS >= BOOTLOADER_ADDRESS - 6)
            {
                // stop writing data to flash if the application is too big, and clear any leftover data in the page buffer
                __boot_page_fill_clear();
                return isLast;
            }

            writeWordToPageBuffer(*(short *)data);
#else
			cli();
            boot_page_fill(CURRENT_ADDRESS, *(short *)data);
            sei();
            CURRENT_ADDRESS += 2;
#endif
            data += 2;
            /* write page when we cross page boundary or we have the last partial page */
            if((currentAddress.w[0] & (SPM_PAGESIZE - 1)) == 0 || (isLast && i >= len && isLastPage)){
                DBG1(0x34, 0, 0);
#ifdef TINY85MODE
                flashPageLoaded = 1;
#else
#	ifndef NO_FLASH_WRITE
                cli();
                boot_page_write(CURRENT_ADDRESS - 2);
                sei();
                boot_spm_busy_wait();
                cli();
                boot_rww_enable();
                sei();
#	endif
#endif
            }
        }
        DBG1(0x35, (void *)&currentAddress.l, 4);
#if HAVE_EEPROM_PAGED_ACCESS
    }
#endif
    return isLast;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
    uchar   i;

    if(len > bytesRemaining)
        len = bytesRemaining;
    bytesRemaining -= len;
    for(i = 0; i < len; i++){
#if HAVE_EEPROM_PAGED_ACCESS
        if(currentRequest >= USBASP_FUNC_READEEPROM){
            *data = eeprom_read_byte((void *)currentAddress.w[0]);
        }else{
#endif
            *data = pgm_read_byte((void *)CURRENT_ADDRESS);

            // read back original vectors
#ifdef TINY85MODE
#if 1
            if(CURRENT_ADDRESS == RESET_VECTOR_OFFSET * 2)
                *data = vectorTemp[0];
            if(CURRENT_ADDRESS == (RESET_VECTOR_OFFSET * 2) + 1)
                *data = vectorTemp[0]/256;
            if(CURRENT_ADDRESS == (USBPLUS_VECTOR_OFFSET * 2))
                *data = vectorTemp[1];
            if(CURRENT_ADDRESS == (USBPLUS_VECTOR_OFFSET * 2) + 1)
                *data = vectorTemp[1]/256;
#else
            if(CURRENT_ADDRESS == RESET_VECTOR_OFFSET * 2 || CURRENT_ADDRESS == USBPLUS_VECTOR_OFFSET * 2)
            {
                *(short *)data = vectorTemp[CURRENT_ADDRESS ? 1:0];
                data++;
                CURRENT_ADDRESS++;
            }
#endif
#endif
#if HAVE_EEPROM_PAGED_ACCESS
        }
#endif
        data++;
        CURRENT_ADDRESS++;
    }
    return len;
}

/* ------------------------------------------------------------------------ */

#ifdef TINY85MODE
void PushMagicWord (void) __attribute__ ((naked)) __attribute__ ((section (".init3")));

// put the word "B007" at the bottom of the stack (RAMEND - RAMEND-1)
void ma (void)
{
    asm volatile("ldi r16, 0xB0"::);
    asm volatile("push r16"::);
    asm volatile("ldi r16, 0x07"::);
    asm volatile("push r16"::);
}
#endif

/* ------------------------------------------------------------------------ */

static inline void initForUsbConnectivity(void)
{
    usbInit();
    /* enforce USB re-enumerate: */
    usbDeviceDisconnect();  /* do this while interrupts are disabled */
    _delay_ms(500);
    usbDeviceConnect();
    sei();
}

#ifdef TINY85MODE
static inline void tiny85FlashInit(void)
{
    // check for erased first page (no bootloader interrupt vectors), add vectors if missing
    if(pgm_read_word(RESET_VECTOR_OFFSET * 2) != 0xC000 + (BOOTLOADER_ADDRESS/2) - 1 ||
            pgm_read_word(USBPLUS_VECTOR_OFFSET * 2) != 0xC000 + (BOOTLOADER_ADDRESS/2) - 1)
    {
        // TODO: actually need to erase here?  how could we end up with missing vectors but application data higher up?
#	if HAVE_CHIP_ERASE
        eraseApplication();
#	else
        fillFlashWithVectors();
#	endif
    }

    // TODO: necessary to reset CURRENT_ADDRESS?
    CURRENT_ADDRESS = 0;
#   ifdef APPCHECKSUM
        checksum = 0;
#   endif
}

static inline void tiny85FlashWrites(void)
{
#if HAVE_CHIP_ERASE
    if(eraseRequested)
    {
        _delay_ms(2);
        cli();
        eraseApplication();
        sei();

#ifdef APPCHECKSUM
        checksum = 0;
#endif
        eraseRequested = 0;
    }
#endif
    if(flashPageLoaded)
    {
        _delay_ms(2);
        // write page to flash, interrupts will be disabled for several milliseconds
        cli();

        if(CURRENT_ADDRESS % SPM_PAGESIZE)
            fillFlashWithVectors();
        else
            writeFlashPage();

        sei();

        flashPageLoaded = 0;
        if(isLastPage)
        {
            // store number of bytes written so rest of flash can be filled later
            writeSize = CURRENT_ADDRESS;
            appWriteComplete = 1;
        }
    }
}
#endif

int __attribute__((noreturn)) main(void)
{
    uint16_t idlePolls = 0;
#ifdef APPCHECKSUM
    int validApp = 0;
#endif

    /* initialize  */
    wdt_disable();      /* main app may have enabled watchdog */
#ifdef TINY85MODE
    tiny85FlashInit();
#endif
    bootLoaderInit();
    odDebugInit();
    DBG1(0x00, 0, 0);

#ifdef APPCHECKSUM
    validApp = testForValidApplication();
#endif

#ifndef TINY85MODE
#	ifndef NO_FLASH_WRITE
    GICR = (1 << IVCE);  /* enable change of interrupt vectors */
    GICR = (1 << IVSEL); /* move interrupts to boot flash section */
#	endif
#endif

    if(bootLoaderCondition()){
        initForUsbConnectivity();
        do{
            usbPoll();
            _delay_us(100);
            idlePolls++;
#ifdef TINY85MODE
            tiny85FlashWrites();
#endif

#if BOOTLOADER_CAN_EXIT
            // exit if requested by the programming app, or if we timeout waiting for the pc with a valid app
            if(requestBootLoaderExit || AUTO_EXIT_CONDITION()){
                _delay_ms(10);
                break;
            }
#endif
        }while(bootLoaderCondition());  /* main event loop */
    }
    leaveBootloader();
}

/* ------------------------------------------------------------------------ */
