@echo Upgrade Digispark Bootloader with micronucleus upload
@if exist upgrade-t167_entry_on_reset_no_pullup.hex  (
  %UserProfile%\AppData\Local\Arduino15\packages\digistump\tools\micronucleus\2.0a4\launcher -cdigispark -Uflash:w:upgrade-t167_entry_on_reset_no_pullup.hex:i
  goto end
)
@rem Try another path
%UserProfile%\AppData\Local\Arduino15\packages\digistump\tools\micronucleus\2.0a4\launcher -cdigispark -Uflash:w:..\firmware\upgrades\upgrade-t167_entry_on_reset_no_pullup.hex:i
:end
pause
