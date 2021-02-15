#include <util/delay_basic.h>
#include <avr/io.h>

// For the niceness
typedef unsigned char byte;
typedef unsigned char boolean;

// make bit & value, eg bit(5) #=> 0b00010000
#define bit(number) _BV(number)
#define pin(number) _BV(number)

// USI serial aliases
#define USIOutputPort    PORTE
#define USIInputPort     PINE
#define USIDirectionPort DDRE
#define USIClockPin      PE4
#define USIDataInPin     PE5
#define USIDataOutPin    PE6

// booleans
#define    on 255
#define   off 0
#define  true 1
#define false 0
#define   yes true
#define    no false

// ensure a value is within the bounds of a minimum and maximum (inclusive)
#define constrainUpper(value, max) (value > max ? max : value)
#define constrainLower(value, min) (value < min ? min : value)
#define constrain(value, min, max) constrainLower(constrainUpper(value, max), min)
#define multiplyDecimal(a,b) (((a) * (b)) / 256)

// set a pin on DDRB to be an input or an output - i.e. becomeOutput(pin(3));
#define inputs(pinmap) DDRB &= ~(pinmap)
#define outputs(pinmap) DDRB |= (pinmap)

// turn some pins on or off
#define pinsOn(pinmap) PORTB |= (pinmap)
#define pinsOff(pinmap) PORTB &= ~(pinmap)
#define pinsToggle(pinmap) PORTB ^= pinmap

// turn a single pin on or off
#define pinOn(pin) pinsOn(bit(pin))
#define pinOff(pin) pinsOff(bit(pin))
// TODO: Should be called pinToggle
#define toggle(pin) pinsToggle(bit(pin))

// delay a number of microseconds - or as close as we can get
#if F_CPU == 16500000
  // special version to deal with half-mhz speed. in a resolution of 2us increments, rounded up
  // this loop has been tuned empirically with an oscilloscope and works in avr-gcc 4.5.1
  static inline void microdelay(int microseconds) {
    while (microseconds > 1) {
      // 16 nops
      asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");
      asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");
      // 16 nops
      asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");asm("NOP");
      asm("NOP");asm("NOP");asm("NOP");


      microseconds -= 2;
    }
  }
#else
  #define microdelay(microseconds) _delay_loop_2(((microseconds) * (F_CPU / 100000)) / 40)
#endif

// delay in milliseconds - a custom implementation to avoid util/delay's tendency to import floating point math libraries
void delay(unsigned int ms) {
  while (ms > 0) {
    // delay for one millisecond (250*4 cycles, multiplied by cpu mhz)
    // subtract number of time taken in while loop and decrement and other bits
    _delay_loop_2((25 * F_CPU / 100000));
    ms--;
  }
}



// digital read returns 0 or 1
#define get(pin) ((PINB >> pin) & 0b00000001)
#define getBitmap(bitmap) (PINB & bitmap)
static inline void set(byte pin, byte state) {
  if (state) { pinOn(pin); } else { pinOff(pin); }
  // alternatly:
  // PORTB = (PORTB & ~(bit(pin)) | ((state & 1) << pin);
}

