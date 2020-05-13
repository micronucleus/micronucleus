@Call SetPath
@echo.
@echo Upgrade Digispark Bootloader with spi programming by avrdude
@if exist t85_default.hex  (
  avrdude -pt85 -cstk500v1 -PCOM6 -b19200 -u -Uflash:w:t85_fast_exit_on_no_USB.hex:a
  goto end
)
@rem Try another path
avrdude -pt85 -cstk500v1 -PCOM6 -b19200 -u -Uflash:w:..\firmware\releases\t85_fast_exit_on_no_USB.hex:a
:end
pause
