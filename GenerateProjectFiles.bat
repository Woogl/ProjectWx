@echo off
"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -ProjectFiles -Project="%~dp0Wx.uproject" -Game -Engine
if %ERRORLEVEL% EQU 0 (
    start "" "%~dp0Wx.sln"
) else (
    echo Project file generation failed.
    pause
)
