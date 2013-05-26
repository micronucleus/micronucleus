
#ifndef LED_H
#define LED_H

#if defined(LED_PORT) && defined(LED_PIN)

#include <hardware.h>
#include "portnames.h"

static inline void led_init (void)
{
	PORT_DDR(LED_PORT) |= _BV(LED_BIT);
}

static inline void __attribute((always_inline)) led_on (void)
{
#if defined(LED_TO_VCC)
	PORT_PORT(LED_PORT) &= ~(_BV(LED_BIT));
#else
	PORT_PORT(LED_PORT) |= _BV(LED_BIT);
#endif
}

static inline void __attribute((always_inline)) led_off (void)
{
#if defined(LED_TO_VCC)
	PORT_PORT(LED_PORT) |= _BV(LED_BIT);
#else
	PORT_PORT(LED_PORT) &= ~(_BV(LED_BIT));
#endif
}

static inline void __attribute((always_inline)) led_toggle(void)
{
	PORT_PORT(LED_PORT) ^= _BV(LED_BIT);
}

#else

#define led_init()
#define led_on()
#define led_off()
#define led_toggle()

#endif

#endif
