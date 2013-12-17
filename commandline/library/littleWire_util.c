#include <littleWire_util.h>

/* Delay in miliseconds */
void delay(unsigned int duration) {
  #if defined _WIN32 || defined _WIN64
    // use windows sleep api with milliseconds
    // * 2 to make it run at half speed, because windows seems to have some trouble with this...
    Sleep(duration * 2);
  #else
    // use standard unix api with microseconds
    usleep(duration*1000);
  #endif
}
