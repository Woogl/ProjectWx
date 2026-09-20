@echo off
setlocal
where pwsh.exe >nul 2>nul
if not errorlevel 1 (
  set "WX_WIKI_PWSH=pwsh.exe"
) else if exist "%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\native\powershell\pwsh.exe" (
  set "WX_WIKI_PWSH=%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\native\powershell\pwsh.exe"
) else (
  echo PowerShell 7 is required. Install it, then reopen this file.
  pause
  exit /b 1
)
"%WX_WIKI_PWSH%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\.agents\scripts\Export-Wiki.ps1" -Open
if errorlevel 1 (
  pause
  exit /b 1
)
