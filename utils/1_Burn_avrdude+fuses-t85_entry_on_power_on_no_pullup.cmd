@echo off
color f0
title AvrDude GUI Command Window
@Call SetPath
REM The files t85_no_pullup.hex and t85_entry_on_power_on_no_pullup.hex are identical!
@echo Upgrade Digispark Bootloader with spi programming by avrdude
@echo.
@echo. Writing ATtiny85 fuses to digispark default except leaving pin5 as reset to enable further low voltage SPI programming
@echo.
@echo. Writing ATtiny85 Lfuse to 0xE1 - (digispark default) PLL Clock + Startup 64 ms
@echo. Writing ATtiny85 Hfuse to 0xDF - External Reset pin enabled (Pin5 not usable as I/O) + BOD disabled + Enable Serial Program and Data Downloading
@echo. Writing ATtiny85 EFuse to 0xFE - self programming enabled.
@echo.
@if exist t85_entry_on_power_on_no_pullup.hex  (
  avrdude -pt85 -cstk500v1 -PCOM6 -b19200 -u -Uflash:w:t85_entry_on_power_on_no_pullup.hex:a -Ulfuse:w:0xE1:m -Uhfuse:w:0xDF:m -Uefuse:w:0xFE:m
  goto end
)
@rem Try another path
avrdude -pt85 -cstk500v1 -PCOM6 -b19200 -u -Uflash:w:..\firmware\releases\t85_entry_on_power_on_no_pullup.hex:a -Ulfuse:w:0xE1:m -Uhfuse:w:0xDF:m -Uefuse:w:0xFE:m
:end
pause
