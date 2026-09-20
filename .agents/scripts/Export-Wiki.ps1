# Copyright Woogle. All Rights Reserved.
#requires -Version 7.0
[CmdletBinding()]
param([string]$RepoRoot, [switch]$Open)
$ErrorActionPreference = 'Stop'
if (!$RepoRoot) { $RepoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent }
$repo = (Resolve-Path -LiteralPath $RepoRoot).Path
$wiki = Join-Path $repo '.agents/wiki'
$manifest = Get-Content -LiteralPath (Join-Path $wiki 'sources.json') -Raw | ConvertFrom-Json -AsHashtable
$documents = @(
    foreach ($folder in @('.agents/wiki', '.agents/reports')) {
        foreach ($file in (Get-ChildItem -LiteralPath (Join-Path $repo $folder) -Recurse -File -Filter '*.md' | Sort-Object FullName)) {
            $path = [IO.Path]::GetRelativePath($repo, $file.FullName).Replace('\', '/')
            $relative = [IO.Path]::GetRelativePath($wiki, $file.FullName).Replace('\', '/')
            $raw = [IO.File]::ReadAllText($file.FullName)
            $heading = [regex]::Match($raw, '(?m)^#\s+(.+)$')
            $meta = $manifest.pages[$relative]
            $category = if ($folder -eq '.agents/reports') { 'reports' } elseif ($relative.Contains('/')) { $relative.Split('/')[0] } else { 'guide' }
            [ordered]@{
                path = $path
                title = if ($heading.Success) { $heading.Groups[1].Value.Trim() } else { $file.BaseName }
                category = $category
                status = if ($meta) { $meta.status } elseif ($category -eq 'reports') { 'historical' } else { 'untracked' }
                scope = if ($meta) { $meta.scope } else { '' }
                text = $raw
                html = (ConvertFrom-Markdown -InputObject $raw).Html
            }
        }
    }
)
$payload = @{ generated = (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'); documents = $documents } | ConvertTo-Json -Depth 8 -Compress
# A document containing </script> must remain data in the generated HTML.
$payload = $payload.Replace('<', '\u003c').Replace('>', '\u003e').Replace('&', '\u0026')
$template = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/index.html'))
$output = Join-Path $repo 'Saved/Wiki/index.html'
New-Item -ItemType Directory -Path (Split-Path $output -Parent) -Force | Out-Null
[IO.File]::WriteAllText($output, $template.Replace('__WX_WIKI_DATA__', $payload), [Text.UTF8Encoding]::new($false))
Write-Output "Wiki viewer: $output ($($documents.Count) documents)"
if ($Open) { Start-Process -FilePath $output }