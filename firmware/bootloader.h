extern const uint8_t bootloader[] PROGMEM;
extern const uint8_t bootloader_end[] PROGMEM;
extern const uint8_t bootloader_size_sym[];
#define bootloader_size ( (int) bootloader_size_sym )
#define bootloader_address 0x1940
