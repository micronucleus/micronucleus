
#ifndef LOW_POWER_INCLUDED
#define LOW_POWER_INCLUDED 1

#include <hardware.h>
#include "portnames.h"

#ifdef LOW_POWER_MODE

#  ifdef SET_CLOCK_PRESCALER

#define store_clkpr()

static inline void restore_clkpr(void)
{
	CLKPR = _BV(CLKPCE);
	CLKPR = SET_CLOCK_PRESCALER;
}

#  else

uint8_t prescaler_default;

static inline void store_clkpr(void)
{
	prescaler_default = CLKPR;
}

static inline void restore_clkpr(void)
{
	CLKPR = 1 << CLKPCE;
	CLKPR = prescaler_default;
}
#  endif

static inline void disable_clkpr(void)
{
	CLKPR = _BV(CLKPCE);
	CLKPR = 0;
}

#undef bootLoaderStartCondition
#define bootLoaderStartCondition()	bit_is_set( PORT_PIN(USB_CFG_IOPORTNAME), USB_CFG_DMINUS_BIT)

#else

#define store_clkpr()
#define restore_clkpr()
#define disable_clkpr()

#endif

#endif
