@echo off
setlocal
echo ========================================
echo  Wx Project Build Test
echo ========================================
echo.

call "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" WxEditor Win64 Development "%~dp0..\Wx.uproject" -WaitMutex

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo  Build Succeeded
    echo ========================================
) else (
    echo.
    echo ========================================
    echo  Build Failed
    echo ========================================
    pause
)
