
#include <littleWire_util.h>

/* Delay in miliseconds */
void delay(unsigned int duration)
{
	#ifdef LINUX
		usleep(duration*1000);
	#else
		Sleep(duration);
	#endif
}
