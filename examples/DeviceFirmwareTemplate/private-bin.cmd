@echo off
rem Tagging and saving private versions of firmware

gitversion /showvariable FullSemVer > tmp.txt
set /P INFOVER=<tmp.txt
del tmp.txt

copy "build\Mokosh.bin" "private\Mokosh-%INFOVER%.bin"