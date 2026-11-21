@echo off
setlocal
cd /d "%~dp0"
chcp 65001 >nul
if not exist "%~dp0tools\taks-world-pc.ps1" (
  echo HIBA: Nem talalhato a tools\taks-world-pc.ps1!
  pause
  exit /b 2
)
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\taks-world-pc.ps1" %*
set "TAKS_EXIT=%ERRORLEVEL%"
if "%~1"=="" (
  if not "%TAKS_EXIT%"=="0" pause
)
exit /b %TAKS_EXIT%
