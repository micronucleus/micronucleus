#ifndef LITTLEWIRE_UTIL_H
#define LITTLEWIRE_UTIL_H

#ifdef LINUX
	#include <unistd.h>
#else
	#include <windows.h>
#endif

/* Delay in miliseconds */
void delay(unsigned int duration);

#endif
