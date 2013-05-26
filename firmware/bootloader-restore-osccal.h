
#ifndef RESTORE_OSCCAL_INCLUDED
#define RESTORE_OSCCAL_INCLUDED 1

#include <hardware.h>

#ifdef RESTORE_OSCCAL

uint8_t osccal_default;

static inline void store_osccal(void)
{
	osccal_default = OSCCAL;
}

static inline void restore_osccal(void)
{
	while (OSCCAL > osccal_default) OSCCAL -= 1;
}

#else

#define store_osccal()
#define restore_osccal()

#endif

#endif
