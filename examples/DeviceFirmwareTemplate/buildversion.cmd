@echo off
gitversion /showvariable semver > tmp.txt
set /P SEMVER=<tmp.txt

gitversion /showvariable informationalversion > tmp.txt
set /P INFOVER=<tmp.txt

"C:\Program Files\Git\usr\bin\date.exe" -Idate > tmp.txt
set /P DATE=<tmp.txt

del tmp.txt

cat BuildVersion.template.h | sed "s/{SemVer}/%SEMVER%/" | sed "s/{InformationalVersion}/%INFOVER%/" | sed "s/{BuildDate}/%DATE%/" > BuildVersion.h