#include <littleWire_util.h>

/* Delay in miliseconds */
void delay(unsigned int duration)
{
	#if defined _WIN32 || defined _WIN64
		// use windows sleep api with milliseconds
		Sleep(duration);
	#else
		// use standard unix api with microseconds
		usleep(duration*1000);
	#endif
}
