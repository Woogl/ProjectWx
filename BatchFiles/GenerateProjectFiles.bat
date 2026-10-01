@rem Copyright Woogle. All Rights Reserved.
@echo off
setlocal

set "ProjectRoot=%~dp0.."
for %%I in ("%ProjectRoot%") do set "ProjectRoot=%%~fI"

set "ProjectFile=%ProjectRoot%\Wx.uproject"
set "SolutionFile=%ProjectRoot%\Wx.sln"
set "EngineResolver=%ProjectRoot%\BatchFiles\Get-WxEngineRoot.ps1"

if not exist "%ProjectFile%" (
    echo Project file not found: %ProjectFile%
    pause
    exit /b 1
)

set "EngineDir="
for /f "usebackq delims=" %%I in (`powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%EngineResolver%" -ProjectFile "%ProjectFile%"`) do set "EngineDir=%%I"
if not defined EngineDir (
    echo Engine resolution failed for: %ProjectFile%
    pause
    exit /b 1
)

set "UnrealBuildTool=%EngineDir%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"

if not exist "%UnrealBuildTool%" (
    echo UnrealBuildTool not found: %UnrealBuildTool%
    pause
    exit /b 1
)

echo Generating Visual Studio 2026 project files for Wx...
pushd "%ProjectRoot%"
"%UnrealBuildTool%" -ProjectFiles -Project="%ProjectFile%" -Game -Engine -2026 -Progress
set "ExitCode=%ERRORLEVEL%"
popd

if not "%ExitCode%"=="0" (
    echo Project file generation failed with exit code %ExitCode%.
    pause
    exit /b %ExitCode%
)

if not exist "%SolutionFile%" (
    echo UnrealBuildTool completed, but the solution file was not created: %SolutionFile%
    pause
    exit /b 1
)

echo Solution generated successfully: %SolutionFile%
exit /b 0
