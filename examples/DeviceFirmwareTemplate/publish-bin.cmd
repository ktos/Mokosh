@echo off
rem Tagging and saving public versions of firmware

gitversion /showvariable informationalversion > tmp.txt
set /P INFOVER=<tmp.txt
del tmp.txt

copy "build\Mokosh.bin" "publish\Mokosh-%INFOVER%.bin"
md5sum "publish\Mokosh-%INFOVER%.bin" > "publish\Mokosh-%INFOVER%.md5"