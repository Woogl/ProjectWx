@rem Copyright Woogle. All Rights Reserved.
@echo off
setlocal

rem Double-click launcher. The real logic lives in .agents/scripts/Export-AbilitySystemLists.ps1, which OpenWiki.bat and AI harnesses run directly.
set "ProjectRoot=%~dp0.."
for %%I in ("%ProjectRoot%") do set "ProjectRoot=%%~fI"
set "ExportScript=%ProjectRoot%\.agents\scripts\Export-AbilitySystemLists.ps1"

if not exist "%ExportScript%" (
    echo Export script not found: %ExportScript%
    pause
    exit /b 2
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%ExportScript%" -RepoRoot "%ProjectRoot%"
set "ExportExitCode=%ERRORLEVEL%"
pause
exit /b %ExportExitCode%
