@rem Copyright Woogle. All Rights Reserved.
@echo off
setlocal

set "ProjectRoot=%~dp0.."
for %%I in ("%ProjectRoot%") do set "ProjectRoot=%%~fI"

set "ProjectFile=%ProjectRoot%\Wx.uproject"
set "SolutionFile=%ProjectRoot%\Wx.sln"
set "LauncherData=%ProgramData%\Epic\UnrealEngineLauncher\LauncherInstalled.dat"

if not exist "%ProjectFile%" (
    echo Project file not found: %ProjectFile%
    pause
    exit /b 1
)

if not exist "%LauncherData%" (
    echo Epic Games Launcher installation data not found: %LauncherData%
    pause
    exit /b 1
)

for /f "usebackq delims=" %%I in (`powershell.exe -NoProfile -Command "$entries = (ConvertFrom-Json ([System.IO.File]::ReadAllText($env:LauncherData))).InstallationList; foreach ($entry in $entries) { if ($entry.AppName -eq 'UE_5.8') { $entry.InstallLocation; break } }"`) do set "EngineDir=%%I"

if not defined EngineDir (
    echo UE 5.8 installation not found in: %LauncherData%
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
