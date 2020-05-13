@Call SetPath
@echo.
@echo Upgrade Digispark Bootloader with spi programming by avrdude
avrdude -pt85 -cstk500v1 -PCOM6 -b19200 -u -Uflash:w:micronucleus-t85_1.06.hex:a
pause