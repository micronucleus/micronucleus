#if 0 // MAKEFILE{

# Arduino.mk config
BOARD_TAG = attiny84
F_CPU = 12800000

#endif // MAKEFILE}

//init2 clears r1 (__zero_reg__) and SREG (disables interrupts) then initializes the stack
//We don't do either and then skip init2 so that we use the bootloader's stack pointer.
//This preserves what the bootloader passes us.
extern "C" __attribute__((naked, section(".init3"))) void __init3() { }
__attribute__((naked, section(".init1"))) void __init1() {
  asm volatile(
//  "\n clr r1"
//  "\n out 0x3f,r1"
  "\n rjmp __init3"
  );

#define STRINGIZE(s) #s
#define EXPAND(s) asm volatile( \
  "\n .global __vector_3" \
  "\n .set __vector_3," STRINGIZE(s) \
  );
  EXPAND(FLASHEND-5)
#undef STRINGIZE
#undef EXPAND
}

void setup()
{
  //must disable wiring.c init messing with OSCCAL before we get here

  //for LED
  pinMode(2, OUTPUT);
}

extern "C" void usbPollLite();
inline void poll()
{
#define STRINGIZE(s) #s
#define EXPAND(s) asm volatile( \
  "\n .global usbPollLite" \
  "\n .set usbPollLite," STRINGIZE(s) \
  );
  usbPollLite();
  EXPAND(FLASHEND-1)
#undef STRINGIZE
#undef EXPAND
}

void loop()
{
  uint32_t time = millis();
  while (millis() - time < 100)
	poll();

  //blink LED
  static uint8_t out;
  digitalWrite(2, out ^= 1);
}
