@echo off
setlocal
rem Opens the Wiki vault in Obsidian. Obsidian only finds vaults it already knows, so add the Wiki folder once with "Open folder as vault".
powershell.exe -NoProfile -Command "Start-Process ('obsidian://open?path=' + [uri]::EscapeDataString((Resolve-Path -LiteralPath '%~dp0..\Wiki\README.md').Path))"
if errorlevel 1 (
  echo Obsidian could not be opened. Install Obsidian, then open the Wiki folder once with "Open folder as vault".
  pause
  exit /b 1
)
