

#ifndef JUMPER_MODE_INCLUDED
#define JUMPER_MODE_INCLUDED 1

#include <hardware.h>
#include "portnames.h"

#ifdef BUILD_JUMPER_MODE

#ifndef START_JUMPER_PORT
#  error "Define START_JUMPER_PORT for your hardware"
#endif
#ifndef START_JUMPER_PIN
#  error "Define START_JUMPER_PIN for your hardware"
#endif


static inline void init_jumper(void)
{
	// enable pull-up
	PORT_PORT(START_JUMPER_PORT) |= _BV(START_JUMPER_PIN);
	_delay_ms(10);
}

static inline void restore_jumper(void)
{
	// disable pull-up
	PORT_PORT(START_JUMPER_PORT) &= ~(_BV(START_JUMPER_PIN));
}

#undef  bootLoaderStartCondition
#define bootLoaderStartCondition() \
	bit_is_set( PORT_PIN(START_JUMPER_PORT), START_JUMPER_PIN)

#undef  bootLoaderCondition
#define bootLoaderCondition() 1

#else

#define init_jumper()
#define restore_jumper()

#endif

#endif
