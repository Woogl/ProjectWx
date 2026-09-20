@rem Copyright Woogle. All Rights Reserved.
@echo off
setlocal

rem Keep the existing entry point; this builds WxEditor, not Visual Studio project files.
set "ProjectRoot=%~dp0.."
for %%I in ("%ProjectRoot%") do set "ProjectRoot=%%~fI"
set "BuildScript=%ProjectRoot%\.agents\skills\build-doctor\scripts\Invoke-WxEditorBuild.ps1"

if not exist "%BuildScript%" (
    echo Build script not found: %BuildScript%
    pause
    exit /b 2
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%BuildScript%" -ProjectRoot "%ProjectRoot%"
set "BuildExitCode=%ERRORLEVEL%"
if not "%BuildExitCode%"=="0" pause
exit /b %BuildExitCode%
