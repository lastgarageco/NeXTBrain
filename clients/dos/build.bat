@echo off
if "%WATT_ROOT%" == "" goto missing
wcl386 -bt=dos -l=dos4g -mf -3r -i=%WATT_ROOT%\inc nbclient.c %WATT_ROOT%\lib\wattcpwf.lib -fe=nbclient.exe
goto end
:missing
echo Set WATT_ROOT to your Watt-32 directory first.
:end
