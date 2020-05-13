@echo off
color f0
title AvrDude Fuses Command Window
@Call SetPath
@echo.
@echo Upgrade Digispark Bootloader with spi programming by avrdude
@echo.
@echo Writing ATtiny85 fuses to digispark default except leaving pin5 as reset to enable further low voltage SPI programming
@echo.
@echo Writing ATtiny85 Lfuse to 0xE1 - (digispark default) PLL Clock + Startup 64 ms
@echo Writing ATtiny85 Hfuse to 0xDF - External Reset pin enabled (Pin5 not usable as I/O) + BOD disabled + Enable Serial Program and Data Downloading
@echo Writing ATtiny85 EFuse to 0xFE - self programming enabled.
@echo.
avrdude -pt85 -cstk500v1 -PCOM6 -b19200 -u -Uflash:w:micronucleus-t85_1.06.hex:a -Ulfuse:w:0xE1:m -Uhfuse:w:0xDF:m -Uefuse:w:0xFE:m
pause