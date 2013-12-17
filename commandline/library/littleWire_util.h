#ifndef LITTLEWIRE_UTIL_H
#define LITTLEWIRE_UTIL_H

#if defined WIN
  #include <windows.h>
#else
  #include <unistd.h>
#endif

/* Delay in miliseconds */
void delay(unsigned int duration);

// end LITTLEWIRE_UTIL_H section:
#endif
