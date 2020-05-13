@echo off
color f0
title AvrDude GUI Command Window
@Call SetPath
@echo.
@echo Upgrade Digispark Bootloader with spi programming by avrdude
@if exist t167_entry_on_power_on_no_pullup.hex  (
  avrdude -pt167 -cstk500v1 -PCOM6 -b19200 -u -Uflash:w:t167_entry_on_power_on_no_pullup.hex:a
  goto end
)
@rem Try another path
avrdude -pt167 -cstk500v1 -PCOM6 -b19200 -u -Uflash:w:..\firmware\releases\t167_entry_on_power_on_no_pullup.hex:a
:end
pause