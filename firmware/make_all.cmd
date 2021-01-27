@echo off
Call SetPath
if not exist releases MKDIR releases
if not exist upgrades MKDIR upgrades

FOR /D  %%C IN (configuration/*) DO (
echo.
echo **********************************************************
echo make Configuration %%C
echo **********************************************************
make clean
make CONFIG=%%C
mv main.hex releases\%%C.hex
mv upgrade.hex upgrades\upgrade-%%C.hex
)
make clean
pause