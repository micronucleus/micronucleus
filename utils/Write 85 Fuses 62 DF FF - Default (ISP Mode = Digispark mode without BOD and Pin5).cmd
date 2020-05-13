@echo off
@Call SetPath
color f0
title AvrDude GUI Command Window
@echo.
@echo Writing ATtiny85 fuses to digispark default except disabling BOD and leaving pin5 as reset to enable ADDITIONAL low voltage SPI programming
@echo.
@echo.Writing ATtiny85 Lfuse to 0x62 - 8MHz clock, 6 Clocks from Power Down sleep + 64 ms after reset.
@echo Writing ATtiny85 Hfuse to 0xDF - External Reset pin enabled (Pin5 not usable as I/O) + BOD disabled + (digispark default) Enable Serial Program and Data Downloading.
@echo Writing ATtiny85 EFuse to 0xFF - self programming disabled - no bootloader function possible.
@echo.
@echo trying to connect to device...
avrdude -p ATtiny85 -c stk500v1 -P COM6  -b 19200 -Ulfuse:w:0x62:m -Uhfuse:w:0xDF:m -Uefuse:w:0xFF:m

pause