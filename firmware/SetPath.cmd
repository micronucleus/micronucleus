@set ARDUINO_DIRECTORY=E:\Elektro\arduino
@echo Set ARDUINO_DIRECTORY to %ARDUINO_DIRECTORY%
@echo Add Arduino binaries and windows "make" executables directory to path
@set PATH=%ARDUINO_DIRECTORY%\hardware\tools\avr\bin;..\windows_exe;%PATH%
rem @echo Add Arduino binaries, Digispark launcher and windows "make" executables directory to path
rem @set PATH=%ARDUINO_DIRECTORY%\hardware\tools\avr\bin;%UserProfile%\AppData\Local\Arduino15\packages\digistump\tools\micronucleus\2.0a4;..\windows_exe;%PATH%
