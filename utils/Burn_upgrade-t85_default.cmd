@echo Upgrade Digispark Bootloader with micronucleus upload
@if exist upgrade-t85_default.hex  (
  %UserProfile%\AppData\Local\Arduino15\packages\digistump\tools\micronucleus\2.0a4\launcher -cdigispark -Uflash:w:upgrade-t85_default.hex:i
  goto end
)
@rem Try another path
%UserProfile%\AppData\Local\Arduino15\packages\digistump\tools\micronucleus\2.0a4\launcher -cdigispark -Uflash:w:..\firmware\upgrades\upgrade-t85_default.hex:i
:end
pause