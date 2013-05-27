

#ifndef LIBMNFLASH_HEXDUMP_H
#define LIBMNFLASH_HEXDUMP_H 1

extern void mnflash_hexdump_header(void);
extern void mnflash_hexdump(int start, uint8_t * buffer, size_t buflen);

#endif
