@echo off
set WEMOS=esp8266:esp8266:d1:xtal=80,baud=921600,eesz=4M3M
set BOARD=%WEMOS%

rem Usage: flash <port> <file>

arduino-cli upload -b %BOARD% -p %1 -i %2