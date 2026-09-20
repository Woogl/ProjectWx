# Copyright Woogle. All Rights Reserved.
[CmdletBinding()]
param(
    [string]$RepoRoot,
    [switch]$Strict
)

$ErrorActionPreference = 'Stop'
try {
    if (!$RepoRoot) { $RepoRoot = Split-Path $PSScriptRoot -Parent }
    $repo = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $wiki = Join-Path $repo '.agents/wiki'
    $manifest = Get-Content -LiteralPath (Join-Path $wiki 'sources.json') -Raw -Encoding UTF8 | ConvertFrom-Json
    $errors = New-Object 'System.Collections.Generic.List[string]'
    $warnings = New-Object 'System.Collections.Generic.List[string]'
    $sources = @{}
    $changed = @{}
    $incoming = @{}
    $registeredPages = @{}
    $commitChecks = @{}

    function Resolve-WxWikiPath([string]$Base, [string]$Relative) {
        $result = [IO.Path]::GetFullPath((Join-Path $Base $Relative))
        if (!$result.StartsWith($repo + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Path outside repository: $Relative"
        }
        return $result
    }

    function Get-WxRelativePath([string]$Path) {
        return $Path.Substring($repo.Length + 1).Replace('\', '/')
    }

    function Test-WxWatchedFile([string]$Path) {
        return $Path -notmatch '(^|/)(Intermediate|Binaries|Saved|DerivedDataCache)/' -and
            $Path -match '\.(h|cpp|cs|ini|uproject|uplugin)$'
    }

    if ($manifest.version -ne 1) { $errors.Add('Unsupported manifest version.') }
    if (!$manifest.sources -or !$manifest.pages) { throw 'Missing sources or pages in manifest.' }
    foreach ($entry in $manifest.sources.PSObject.Properties) {
        $sources[$entry.Name] = $entry.Value
        if ($entry.Value.sha256 -notmatch '^[0-9a-fA-F]{64}$') { $errors.Add("Invalid hash: $($entry.Name)") }
        $path = Resolve-WxWikiPath $repo $entry.Name
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) {
            $changed[$entry.Name] = 'missing'
        } elseif ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.Value.sha256) {
            $changed[$entry.Name] = 'changed'
        }
    }

    foreach ($entry in $manifest.pages.PSObject.Properties) {
        $name = $entry.Name
        $page = $entry.Value
        $path = Resolve-WxWikiPath $wiki $name
        $registeredPages[$path] = $true
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { $errors.Add("Missing page: $name") }
        if ($page.status -notin @('current', 'needs-review', 'blocked', 'superseded')) { $errors.Add("Invalid status: $name") }
        if (!$page.scope -or @($page.sources).Count -eq 0) { $errors.Add("Missing scope/sources: $name") }
        if ($page.status -in @('needs-review', 'blocked')) { $warnings.Add("STATUS $name : $($page.status)") }
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $body = Get-Content -LiteralPath $path -Raw -Encoding UTF8
            if ($body -notmatch ('상태:\s*' + [regex]::Escape($page.status) + '\b')) { $errors.Add("Status differs from manifest: $name") }
        }
        $commit = $page.baseline_commit
        if ($commit) {
            if ($commit -notmatch '^[0-9a-fA-F]{7,40}$') { $errors.Add("Invalid baseline commit: $name") }
            else {
                if (!$commitChecks.ContainsKey($commit)) {
                    $savedPreference = $ErrorActionPreference
                    $ErrorActionPreference = 'Continue'
                    & git -c ('safe.directory=' + $repo.Replace('\', '/')) -C $repo cat-file -e "${commit}^{commit}" 2>$null
                    $commitChecks[$commit] = ($LASTEXITCODE -eq 0)
                    $ErrorActionPreference = $savedPreference
                }
                if (!$commitChecks[$commit]) { $warnings.Add("BASELINE unavailable: $name ($commit)") }
            }
        }
        foreach ($source in @($page.sources)) {
            if (!$sources.ContainsKey($source)) { $errors.Add("Unregistered source: $name -> $source") }
            elseif ($changed.ContainsKey($source)) { $warnings.Add("SOURCE $name : $($changed[$source]) $source") }
        }
        foreach ($watchRoot in @($page.watch_roots)) {
            if (!$watchRoot) { continue }
            $watchPath = Resolve-WxWikiPath $repo $watchRoot
            if (!(Test-Path -LiteralPath $watchPath)) { $warnings.Add("WATCH missing: $name -> $watchRoot"); continue }
            foreach ($file in Get-ChildItem -LiteralPath $watchPath -File -Recurse) {
                $relative = Get-WxRelativePath $file.FullName
                if ((Test-WxWatchedFile $relative) -and $relative -notin @($page.sources)) {
                    $warnings.Add("NEW $name : $relative")
                }
            }
        }
    }

    $wikiPages = @(Get-ChildItem -LiteralPath $wiki -Filter '*.md' -File -Recurse)
    $reportRoot = Join-Path $repo '.agents/reports'
    $reportPages = @()
    if (Test-Path -LiteralPath $reportRoot) {
        $reportPages = @(Get-ChildItem -LiteralPath $reportRoot -Filter '*.md' -File -Recurse)
    }
    $documentPages = @($wikiPages) + @($reportPages)
    $linkFiles = @($documentPages) + @(Get-Item -LiteralPath (Join-Path $repo 'README.md') -ErrorAction SilentlyContinue)
    foreach ($page in $manifest.pages.PSObject.Properties) {
        if ($page.Value.legacy_readme) {
            $legacy = Resolve-WxWikiPath $repo $page.Value.legacy_readme
            if (Test-Path -LiteralPath $legacy) { $linkFiles += Get-Item -LiteralPath $legacy }
            else { $errors.Add("Missing README entry: $legacy") }
        }
    }
    foreach ($file in $linkFiles) {
        $body = Get-Content -LiteralPath $file.FullName -Raw -Encoding UTF8
        $body = [regex]::Replace($body, '(?ms)^```.*?^```[^\r\n]*', '')
        if ($body -match '\[\[[^\]]+\]\]') { $errors.Add("Unresolved wikilink: $($file.FullName)") }
        foreach ($match in [regex]::Matches($body, '\[[^\]\r\n]*\]\(([^)\r\n]+)\)')) {
            $target = $match.Groups[1].Value.Trim().Trim('<', '>')
            if ($target -match '^(https?://|mailto:|#)') { continue }
            $target = [Uri]::UnescapeDataString(($target -split '#', 2)[0])
            if (!$target) { continue }
            $resolved = Resolve-WxWikiPath $file.DirectoryName $target
            if (!(Test-Path -LiteralPath $resolved)) { $errors.Add("Broken link: $($file.FullName) -> $target") }
            elseif ($resolved -ne $file.FullName) { $incoming[$resolved] = $true }
        }
    }
    foreach ($file in $documentPages) {
        if ($file.FullName -ne (Join-Path $wiki 'index.md') -and !$incoming.ContainsKey($file.FullName)) {
            $errors.Add("Orphan page: $($file.FullName)")
        }
        if ($file.FullName -match '[\\/](modules|systems|questions|decisions)[\\/]' -and !$registeredPages.ContainsKey($file.FullName)) {
            $errors.Add("Unregistered page: $($file.FullName)")
        }
    }
    foreach ($entry in $errors) { Write-Output "ERROR $entry" }
    foreach ($entry in $warnings) { Write-Output "REVIEW $entry" }
    Write-Output "Wiki check: $($errors.Count) errors, $($warnings.Count) review notices, $($manifest.pages.PSObject.Properties.Name.Count) registered pages."
    if ($errors.Count -gt 0 -or ($Strict -and $warnings.Count -gt 0)) { exit 1 }
    exit 0
} catch {
    Write-Output "TOOL ERROR: $($_.Exception.Message)"
    exit 2
}
