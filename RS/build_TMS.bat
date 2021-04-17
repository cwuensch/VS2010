@echo off
cd /d %~dp0
if "%TFROOT%"=="" set TFROOT=C:\sw\prgm\Topfield
set PATH=%TFROOT%\gccForTMS\crosstool\bin;%TFROOT%\Cygwin\bin;C:\sw\OS\cygwin\bin;%PATH%
rem del /Q bin obj 2> nul
rem bash -i -c make

if "%1"=="/debug" (
  set MakeParam=--makefile=Makedebug
) else (
  set MakeParam= 
)
make %MakeParam%
set BuildState=%errorlevel%
del *.d
del *.o

copy /y RecStrip_VS\Release\RecStrip.exe RecStrip_Win32.exe
copy /y RecStrip_VS\x64\Release\RecStrip.exe RecStrip_x64.exe

if not "%1"=="/quiet" (
  if not "%2"=="/quiet" (
    pause
  )
)
exit %BuildState%
