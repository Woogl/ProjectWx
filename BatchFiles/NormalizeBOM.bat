@rem Copyright Woogle. All Rights Reserved.
@echo off
setlocal
set "WxBomScript=%~f0"
set "WxBomRoot=%~dp0.."
set "WxBomCheck=0"
if /i "%~1"=="--check" (
    set "WxBomCheck=1"
) else if not "%~1"=="" (
    echo Usage: NormalizeBOM.bat [--check]
    exit /b 2
)
if not "%~2"=="" (
    echo Usage: NormalizeBOM.bat [--check]
    exit /b 2
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$script = [IO.File]::ReadAllText($env:WxBomScript); & ([scriptblock]::Create($script.Substring($script.LastIndexOf('# POWERSHELL PAYLOAD') + 20)))"
exit /b %ERRORLEVEL%
# POWERSHELL PAYLOAD
$ErrorActionPreference = 'Stop'

function Get-SourceFiles([string]$Path, [bool]$InSource) {
    if (-not (Test-Path -LiteralPath $Path -PathType Container)) { return }
    foreach ($entry in Get-ChildItem -LiteralPath $Path -Force) {
        # Do not follow links outside the source tree or touch generated/vendor files.
        if ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) { continue }
        if ($entry.PSIsContainer) {
            if ($entry.Name -in @('Binaries', 'Intermediate', 'Saved', 'DerivedDataCache', 'ThirdParty', '.git')) { continue }
            Get-SourceFiles $entry.FullName ($InSource -or $entry.Name -eq 'Source')
        } elseif ($InSource -and $entry.Extension -in @('.h', '.cpp', '.cs')) {
            $entry
        }
    }
}

try {
    $root = [IO.Path]::GetFullPath($env:WxBomRoot)
    if (-not (Test-Path -LiteralPath (Join-Path $root 'Wx.uproject') -PathType Leaf)) {
        throw 'Wx.uproject not found next to BatchFiles.'
    }
    $checkOnly = $env:WxBomCheck -eq '1'
    $utf8 = New-Object System.Text.UTF8Encoding($false, $true)
    $files = @(Get-SourceFiles (Join-Path $root 'Source') $true)
    $files += @(Get-SourceFiles (Join-Path $root 'Plugins') $false)
    $bomCount = 0
    $changed = 0
    $errors = 0
    foreach ($file in $files) {
        try {
            $bytes = [IO.File]::ReadAllBytes($file.FullName)
            $offset = 0
            # Strip every leading UTF-8 signature, including accidentally duplicated BOMs.
            while ($bytes.Length -ge ($offset + 3) -and $bytes[$offset] -eq 0xEF -and $bytes[$offset + 1] -eq 0xBB -and $bytes[$offset + 2] -eq 0xBF) {
                $offset += 3
            }
            $null = $utf8.GetCharCount($bytes, $offset, $bytes.Length - $offset)
            # BOM-less UTF-16/32 ASCII can pass UTF-8 validation; NUL is not source text.
            if ($bytes -contains 0) { throw 'NUL bytes found; expected UTF-8 source text.' }
            if ($offset -eq 0) { continue }
            $bomCount++
            if ($checkOnly) {
                Write-Host ('BOM: ' + $file.FullName)
            } else {
                $payload = New-Object byte[] ($bytes.Length - $offset)
                [Array]::Copy($bytes, $offset, $payload, 0, $payload.Length)
                [IO.File]::WriteAllBytes($file.FullName, $payload)
                $changed++
                Write-Host ('FIXED: ' + $file.FullName)
            }
        } catch {
            $errors++
            [Console]::Error.WriteLine('ERROR: {0}: {1}', $file.FullName, $_.Exception.Message)
        }
    }
    Write-Host ('Scanned: {0}; BOM: {1}; Changed: {2}; Errors: {3}' -f $files.Count, $bomCount, $changed, $errors)
    # 0 = compliant, 1 = check found BOM, 2 = invalid input or processing error.
    if ($errors -gt 0) { exit 2 }
    if ($checkOnly -and $bomCount -gt 0) { exit 1 }
    exit 0
} catch {
    [Console]::Error.WriteLine('ERROR: ' + $_.Exception.Message)
    exit 2
}
