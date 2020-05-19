@echo off
rem Uploads configuration file to the connected Mokosh device
curl -F "data=@config.json" http://192.168.4.1/config