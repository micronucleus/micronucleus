#ifndef PORT_NAMES
#define PORT_NAMES 1

#include <avr/sfr_defs.h>

#define PORT_CONCAT(a, b)	a ## b
#define PORT_DDR(a)		PORT_CONCAT(DDR, a)
#define PORT_PIN(a)		PORT_CONCAT(PIN, a)
#define PORT_PORT(a)		PORT_CONCAT(PORT, a)

#endif

