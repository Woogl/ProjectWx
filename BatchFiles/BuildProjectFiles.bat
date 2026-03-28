@echo off
setlocal

set "ProjectFile=%~dp0..\Wx.uproject"

if not exist "%ProjectFile%" (
    echo Project file not found: %ProjectFile%
    pause
    exit /b 1
)

for /f "tokens=2 delims=:," %%A in (
    'findstr /C:"EngineAssociation" "%ProjectFile%"'
) do (
    set "RawVersion=%%~A"
)
for /f "tokens=*" %%A in ("%RawVersion%") do set "EngineVersion=%%~A"

if not defined EngineVersion (
    echo Failed to read EngineAssociation from %ProjectFile%.
    pause
    exit /b 1
)

for /f "tokens=2*" %%A in (
    'reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\%EngineVersion%" /v "InstalledDirectory" 2^>nul'
) do set "EngineDir=%%B"

if not defined EngineDir (
    echo Unreal Engine %EngineVersion% installation not found in registry.
    pause
    exit /b 1
)

call "%EngineDir%\Engine\Build\BatchFiles\Build.bat" WxEditor Win64 Development "%ProjectFile%" -WaitMutex

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
