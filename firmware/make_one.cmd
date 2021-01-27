@echo off
Call SetPath
if not exist releases MKDIR releases
if not exist upgrades MKDIR upgrades

rem Set TARGET=t85_entry_on_powerOn
rem Set TARGET=t85_entry_on_power_on_activePullup_fastExit
rem Set TARGET=t85_entry_on_reset_activePullup
rem Set TARGET=t85_entry_on_powerOn_activePullup_fastExit
Set TARGET=t167_default

echo.
echo **********************************************************
echo make Configuration %TARGET%
echo **********************************************************
make clean
make CONFIG=%TARGET%
mv main.hex releases\%TARGET%.hex
mv upgrade.hex upgrades\upgrade-%TARGET%.hex

pause
make clean