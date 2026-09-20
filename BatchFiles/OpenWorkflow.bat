@echo off
setlocal
rem The second candidate is the PowerShell 7 bundled with the Codex runtime: a fallback for machines without pwsh on PATH.
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
"%WX_WIKI_PWSH%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\.agents\scripts\Start-WikiAI.ps1"
if errorlevel 1 echo AI connection unavailable. Document browsing is still available.
"%WX_WIKI_PWSH%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\.agents\scripts\Export-Wiki.ps1" -View Workflow -Open
if errorlevel 1 (
  pause
  exit /b 1
)
