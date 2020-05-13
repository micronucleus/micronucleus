@echo off
@Call ../firmware/SetPath
color f0
title AvrDude GUI Command Window
@echo.
@echo. trying to connect to device...
avrdude -p ATtiny85 -c stk500v1 -P COM6  -b 19200 -U hfuse:r:"fuse_bits.raw":r
pause