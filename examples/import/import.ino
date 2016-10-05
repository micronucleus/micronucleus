#if 0 // MAKEFILE{

# Arduino.mk config
BOARD_TAG = attiny84
F_CPU = 12800000

#endif // MAKEFILE}

#include "unusb.hpp"

void setup()
{
  //must disable wiring.c init messing with OSCCAL before we get here
  unUsb.begin(true);

  //for LED
  pinMode(2, OUTPUT);
}

void loop()
{
  unUsb.poll(500);

  //blink LED
  static uint8_t out = '0';

  if (unUsb.available())
	out = unUsb.read();
  unUsb.write(out);

  digitalWrite(2, out & 1);
  out ^= 1;
}
