
#ifndef TIMER_MODE_INCLUDED
#define TIMER_MODE_INCLUDED 1

#include <hardware.h>
#include "portnames.h"

#ifdef TIMER_MODE

#ifndef AUTO_EXIT_MS
#  define AUTO_EXIT_MS    5000
#endif

#if AUTO_EXIT_MS < (MICRONUCLEUS_WRITE_SLEEP * (BOOTLOADER_ADDRESS / SPM_PAGESIZE))
#    warning "AUTO_EXIT_MS is shorter than the time it takes to perform erase function - might affect reliability?"
#    warning "Try increasing AUTO_EXIT_MS if you have stability problems"
#endif

uint16_t idlePolls = 0;

static inline void incr_idle_polls(void)
{
	idlePolls ++;
}

static inline void reset_idle_polls(void)
{
	idlePolls = 0;
}

static inline void two_more_idle_polls(void)
{
	idlePolls = ((AUTO_EXIT_MS - 21) * 10UL);
}

#undef  bootLoaderStartCondition
#define bootLoaderStartCondition() 1

#undef  bootLoaderCondition
#define bootLoaderCondition() (idlePolls < (AUTO_EXIT_MS * 10UL))

#else

#define incr_idle_polls()
#define reset_idle_polls()
#define two_more_idle_polls()

#endif

#endif
