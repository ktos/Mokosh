@echo off
rem Wemos D1
set WEMOS=esp8266:esp8266:d1:xtal=80,baud=921600,eesz=4M3M
set BOARD=%WEMOS%

arduino-cli compile -b %BOARD% -o build\Mokosh.bin --warnings all