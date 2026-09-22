# Copyright Woogle. All Rights Reserved.
[CmdletBinding()]
param([string]$RepoRoot)
$ErrorActionPreference = 'Stop'
try {
    if (!$RepoRoot) { $RepoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent }
    $repo = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    # Raw preserves imported source text and its original relative paths; lint articles, not immutable inputs.
    $files = @(foreach ($folder in @('.wiki/wiki', '.agents/workflow')) {
        if (!(Test-Path -LiteralPath (Join-Path $repo $folder))) { continue }
        Get-ChildItem -LiteralPath (Join-Path $repo $folder) -Filter '*.md' -File -Recurse
    })
    $files += @(Get-ChildItem -LiteralPath (Join-Path $repo '.wiki') -Filter '*.md' -File)
    $files += @(Get-Item -LiteralPath (Join-Path $repo 'README.md') -ErrorAction SilentlyContinue)
    foreach ($folder in @('Plugins', 'Source')) {
        foreach ($module in Get-ChildItem -LiteralPath (Join-Path $repo $folder) -Directory -ErrorAction SilentlyContinue) {
            $files += @(Get-Item -LiteralPath (Join-Path $module.FullName 'README.md') -ErrorAction SilentlyContinue)
        }
    }
    $errors = 0
    foreach ($file in $files) {
        $body = Get-Content -LiteralPath $file.FullName -Raw -Encoding UTF8
        $body = [regex]::Replace($body, '(?ms)^```.*?^```[^\r\n]*', '')
        foreach ($match in [regex]::Matches($body, '\[[^\]\r\n]*\]\((<[^>\r\n]+>|[^)\r\n]+)\)')) {
            $target = $match.Groups[1].Value.Trim().Trim('<', '>')
            if ($target -match '^([a-zA-Z][a-zA-Z0-9+.-]*:|//|#)') { continue }
            $target = [Uri]::UnescapeDataString(($target -split '#', 2)[0])
            if (!$target) { continue }
            $resolved = [IO.Path]::GetFullPath((Join-Path $file.DirectoryName $target))
            if (!$resolved.StartsWith($repo + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or !(Test-Path -LiteralPath $resolved)) {
                Write-Output "ERROR Link: $($file.FullName) -> $target"
                $errors++
            }
        }
    }
    Write-Output "Wiki links: $errors errors, $($files.Count) documents."
    if ($errors) { exit 1 }
    exit 0
} catch {
    Write-Output "TOOL ERROR: $($_.Exception.Message)"
    exit 2
}
