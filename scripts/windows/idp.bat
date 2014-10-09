@echo off
setlocal
set PATH=%~dp0;%~dp0..\lib\;%path%
"%~dp0kbs.exe" -d "%~dp0..\\" %*
endlocal