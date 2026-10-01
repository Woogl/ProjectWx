@rem Copyright Woogle. All Rights Reserved.
@echo off
setlocal

set "ProjectRoot=%~dp0.."
for %%I in ("%ProjectRoot%") do set "ProjectRoot=%%~fI"
set "CheckScript=%ProjectRoot%\.agents\scripts\Check-Redirects.ps1"

if not exist "%CheckScript%" (
    echo Check script not found: "%CheckScript%"
    pause
    exit /b 2
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%CheckScript%" -ProjectRoot "%ProjectRoot%" -Interactive
set "CheckExitCode=%ERRORLEVEL%"
pause
exit /b %CheckExitCode%
