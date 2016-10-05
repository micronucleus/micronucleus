#ifndef UnUsb_hpp
#define UnUsb_hpp

#define RING_BUFFER_SIZE 8

struct ring_buffer {
  unsigned char buffer[RING_BUFFER_SIZE];
  unsigned char head, tail;
} tx_buffer = { { 0 }, 0, 0 };
int rx_buffer = -1;

class UnUsb : public Print {
 public:
  void begin(bool _block = false) { block = _block; }

  void poll(unsigned long milliseconds);

  size_t write(byte c);
  bool available() { return rx_buffer != -1; }
  int peek() { return rx_buffer; }
  int read() { int r = rx_buffer; rx_buffer = -1; return r; }

  bool block = false;
};

UnUsb unUsb;

inline void unUsbPoll();

// wait a specified number of milliseconds (roughly), refreshing in the background
void UnUsb::poll(unsigned long milli) {
  uint32_t start = micros();
  do {
    while (milli > 0 && (micros() - start) >= 1000) {
      milli--;
      start += 1000;
    }
	unUsbPoll();
  } while (milli > 0);
}

size_t UnUsb::write(byte c)
{
  while (block && tx_buffer.head != tx_buffer.tail) { unUsbPoll(); }
  int i = (tx_buffer.head + 1) % RING_BUFFER_SIZE;
  if (i == tx_buffer.tail)
    return 0;

  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
    tx_buffer.buffer[tx_buffer.head] = c;
    tx_buffer.head = i;
  while (block && tx_buffer.head != tx_buffer.tail) { unUsbPoll(); }
    return 1;
}

struct usbRequest {
    uint8_t    bmRequestType;
    uint8_t    bRequest;
    uint16_t   wValue;
    uint16_t   wIndex;
    uint16_t   wLength;
};

uint8_t usbFunctionSetup(uint8_t data[8])
{
	usbRequest& rq = *(usbRequest*)data;

    if ((rq.bmRequestType & 0x60) != (1<<5))
		// ignore vendor type requests, we don't use any
		return 0;

	if (rq.bRequest != 137)
		return 0;

	if (rq.bmRequestType & 0x80)
	{
		// USBRQ_DIR_DEVICE_TO_HOST
		if (tx_buffer.head == tx_buffer.tail)
			return 0;

		// hack!
//		usbMsgPtr = (short unsigned int)&buf;
		data[10] = tx_buffer.buffer[tx_buffer.tail];
		tx_buffer.tail = (tx_buffer.tail + 1) % RING_BUFFER_SIZE;
		return 1;
	}
	else
	{
		// USBRQ_DIR_HOST_TO_DEVICE
		memcpy((char*)&rx_buffer, &rq.wValue, sizeof(rx_buffer));
		return 0;
	}
}

extern "C" void usbPollLite(uint8_t (*usbFunctionSetup)(uint8_t data[8]));
inline void unUsbPoll()
{
#define STRINGIZE(s) #s
#define EXPAND(s) asm volatile( \
  "\n .global usbPollLite" \
  "\n .set usbPollLite," STRINGIZE(s) \
  );
  usbPollLite(usbFunctionSetup);
  EXPAND(FLASHEND-1)
#undef STRINGIZE
#undef EXPAND
}

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

#endif // UnUsb_hpp
