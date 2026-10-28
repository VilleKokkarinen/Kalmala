@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Render-All-Previews.ps1"
set "renderExit=%ERRORLEVEL%"
echo.
if not "%renderExit%"=="0" echo Preview batch failed. See the output above.
pause
exit /b %renderExit%
