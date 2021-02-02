@echo off
where /q rm
IF ERRORLEVEL 1 (Call SetPath)
make clean
