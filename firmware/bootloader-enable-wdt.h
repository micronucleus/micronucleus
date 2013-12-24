
#ifndef WDT_INCLUDET
#define WDT_INCLUDET 1

#include <hardware.h>
#include <avr/wdt.h>

#ifdef ENABLE_WDT
static inline void enable_wdt (void)
{
	wdt_enable(WDTO_2S);
}
#else

#define enable_wdt()

#endif

#endif
