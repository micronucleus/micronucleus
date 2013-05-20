

#ifndef HEXDUMP_H
#define HEXDUMP_H 1

extern void hexdump_header(void);
extern void hexdump(int start, uint8_t * buffer, size_t buflen);

#endif
