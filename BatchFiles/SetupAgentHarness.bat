@rem Copyright Woogle. All Rights Reserved.
@echo off
setlocal

rem Double-click launcher. The real logic lives in .agents/scripts/Setup-AgentHarness.ps1, which AI harnesses run directly.
set "ProjectRoot=%~dp0.."
for %%I in ("%ProjectRoot%") do set "ProjectRoot=%%~fI"
set "SetupScript=%ProjectRoot%\.agents\scripts\Setup-AgentHarness.ps1"

if not exist "%SetupScript%" (
    echo Setup script not found: %SetupScript%
    pause
    exit /b 2
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SetupScript%" -RepoRoot "%ProjectRoot%"
set "SetupExitCode=%ERRORLEVEL%"
pause
exit /b %SetupExitCode%
