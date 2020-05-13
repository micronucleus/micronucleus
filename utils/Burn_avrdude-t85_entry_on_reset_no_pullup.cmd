@Call SetPath
@echo.
@echo Upgrade Digispark Bootloader with spi programming by avrdude
@if exist t85_entry_on_reset_no_pullup.hex  (
  avrdude -pt85 -cstk500v1 -PCOM6 -b19200 -u -Uflash:w:t85_entry_on_reset_no_pullup.hex:a
  goto end
)
@rem Try another path
avrdude -pt85 -cstk500v1 -PCOM6 -b19200 -u -Uflash:w:..\firmware\releases\t85_entry_on_reset_no_pullup.hex:a
:end
pause
