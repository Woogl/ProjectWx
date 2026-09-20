# Copyright Woogle. All Rights Reserved.
#requires -Version 7.0
[CmdletBinding()]
param([string]$RepoRoot, [switch]$Open, [ValidateSet('Wiki', 'Workflow')][string]$View = 'Wiki')
$ErrorActionPreference = 'Stop'
if (!$RepoRoot) { $RepoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent }
$repo = (Resolve-Path -LiteralPath $RepoRoot).Path
$wiki = Join-Path $repo '.agents/wiki'
$manifest = Get-Content -LiteralPath (Join-Path $wiki 'sources.json') -Raw | ConvertFrom-Json -AsHashtable
$documents = @(
    foreach ($folder in @('.agents/wiki', '.agents/in-progress')) {
        if (!(Test-Path -LiteralPath (Join-Path $repo $folder))) { continue }
        foreach ($file in (Get-ChildItem -LiteralPath (Join-Path $repo $folder) -Recurse -File -Filter '*.md' | Sort-Object FullName)) {
            $path = [IO.Path]::GetRelativePath($repo, $file.FullName).Replace('\', '/')
            $relative = [IO.Path]::GetRelativePath($wiki, $file.FullName).Replace('\', '/')
            $raw = [IO.File]::ReadAllText($file.FullName)
            $heading = [regex]::Match($raw, '(?m)^#\s+(.+)$')
            $meta = $manifest.pages[$relative]
            $stageLabels = @{ '기획' = 'planning'; '구현' = 'implementation'; '테스트' = 'testing'; '완료' = 'completion' }
            $stageLine = [regex]::Match($raw, '(?m)^작업 단계:[ \t]*(.+)\r?$')
            $stages = @(if ($stageLine.Success) {
                foreach ($label in ($stageLine.Groups[1].Value -split ',')) {
                    $label = $label.Trim()
                    if (!$stageLabels.ContainsKey($label)) { throw "Unknown workflow stage '$label': $path" }
                    $stageLabels[$label]
                }
            })
            $category = if ($folder -eq '.agents/in-progress') { 'in-progress' } elseif ($relative.Contains('/')) { $relative.Split('/')[0] } else { 'guide' }
            [ordered]@{
                path = $path
                title = if ($heading.Success) { $heading.Groups[1].Value.Trim() } else { $file.BaseName }
                category = $category
                stages = $stages
                status = if ($meta) { $meta.status } elseif ($category -eq 'in-progress') { 'historical' } else { 'untracked' }
                scope = if ($meta) { $meta.scope } else { '' }
                text = $raw
                html = (ConvertFrom-Markdown -InputObject $raw).Html
            }
        }
    }
)
$connectionPath = Join-Path $repo 'Saved/Wiki/ai-connection.json'
$ai = if (Test-Path -LiteralPath $connectionPath) { Get-Content -LiteralPath $connectionPath -Raw | ConvertFrom-Json -AsHashtable } else { $null }
# Bundle Wiki PNG assets so the file viewer needs no external image access.
$images = @{}
$assetRoot = Join-Path $wiki 'assets'
if (Test-Path -LiteralPath $assetRoot) {
    foreach ($asset in (Get-ChildItem -LiteralPath $assetRoot -File -Filter '*.png')) {
        if ($asset.Attributes -band [IO.FileAttributes]::ReparsePoint) { continue }
        $bytes = [IO.File]::ReadAllBytes($asset.FullName)
        if ($bytes.Length -lt 8 -or [Convert]::ToHexString($bytes[0..7]) -ne '89504E470D0A1A0A') { throw "Invalid PNG asset: $($asset.Name)" }
        $key = [IO.Path]::GetRelativePath($repo, $asset.FullName).Replace('\', '/')
        $images[$key] = 'data:image/png;base64,' + [Convert]::ToBase64String($bytes)
    }
}
$template = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/index.html'))
$workflowScript = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/workflow-model.js')) + "`n" + [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/workflow.js')) + "`n" + [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/execution.js'))
foreach ($mode in @('Wiki', 'Workflow')) {
    $indexPath = if ($mode -eq 'Wiki') { Join-Path $wiki 'index.md' } else { Join-Path $wiki 'workflow/index.md' }
    $navigation = @()
    $group = $null
    foreach ($line in (Get-Content -LiteralPath $indexPath)) {
        if ($line -match '^## (.+)$') {
            $group = [ordered]@{ title = $Matches[1]; items = @() }
            $navigation += $group
        } elseif ($group -and $line -match '^\s*(?:-|\d+\.) \[([^\]]+)\]\(([^)]+)\)$') {
            $target = [IO.Path]::GetFullPath((Join-Path (Split-Path $indexPath -Parent) $Matches[2]))
            $group.items += @{ title = $Matches[1]; path = [IO.Path]::GetRelativePath($repo, $target).Replace('\', '/') }
        }
    }
    $connection = if ($mode -eq 'Workflow') { $ai } else { $null }
    $payload = @{ mode = $mode; ai = $connection; images = $images; generated = (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'); documents = $documents; navigation = $navigation } | ConvertTo-Json -Depth 8 -Compress
    $payload = $payload.Replace('<', '\u003c').Replace('>', '\u003e').Replace('&', '\u0026')
    $script = if ($mode -eq 'Workflow') { $workflowScript } else { "function renderWorkSummary() {} function renderWorkflow() {} function onWikiNavigation() { readRoute(); }" }
    $page = $template.Replace('__WX_WORKFLOW_SCRIPT__', $script).Replace('__WX_WIKI_DATA__', $payload)
    if ($mode -eq 'Wiki') { $page = $page.Replace('connect-src http://127.0.0.1:18743', "connect-src 'none'") }
    # 기존 파일 URL과 저장 키를 유지하여 미확정 판단·저장 복구 기록을 옮기지 않는다.
    $name = if ($mode -eq 'Workflow') { 'index.html' } else { 'knowledge.html' }
    $output = Join-Path $repo ('Saved/Wiki/' + $name)
    New-Item -ItemType Directory -Path (Split-Path $output -Parent) -Force | Out-Null
    [IO.File]::WriteAllText($output, $page, [Text.UTF8Encoding]::new($false))
    Write-Output "$mode viewer: $output ($($documents.Count) documents)"
    if ($Open -and $View -eq $mode) { Start-Process -FilePath $output }
}
