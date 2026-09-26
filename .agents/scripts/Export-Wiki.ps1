# Copyright Woogle. All Rights Reserved.
#requires -Version 7.0
[CmdletBinding()]
param([string]$RepoRoot, [switch]$Open)
$ErrorActionPreference = 'Stop'
if (!$RepoRoot) { $RepoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent }
$repo = (Resolve-Path -LiteralPath $RepoRoot).Path
$workflow = Join-Path $repo '.agents/workflow'
$documents = @(
    foreach ($file in (Get-ChildItem -LiteralPath $workflow -Recurse -File -Filter '*.md' | Sort-Object FullName)) {
        $raw = [IO.File]::ReadAllText($file.FullName)
        # Keep YAML verbatim for provenance display; rendering never interprets it as document headings or a second schema.
        $frontmatter = [regex]::Match($raw, '\A---\r?\n([\s\S]*?)\r?\n---(?:\r?\n|$)')
        $body = if ($frontmatter.Success) { $raw.Substring($frontmatter.Length) } else { $raw }
        $heading = [regex]::Match($body, '(?m)^#\s+(.+)$')
        [ordered]@{
            path = [IO.Path]::GetRelativePath($repo, $file.FullName).Replace('\', '/')
            title = if ($heading.Success) { $heading.Groups[1].Value.Trim() } else { $file.BaseName }
            frontmatter = if ($frontmatter.Success) { $frontmatter.Groups[1].Value } else { '' }
            text = $raw
            modified = $file.LastWriteTimeUtc.ToString('o')
            html = (ConvertFrom-Markdown -InputObject $body).Html
        }
    }
)
$indexPath = Join-Path $workflow 'index.md'
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
$connectionPath = Join-Path $repo 'Saved/Workflow/ai-connection.json'
$ai = if (Test-Path -LiteralPath $connectionPath) { Get-Content -LiteralPath $connectionPath -Raw | ConvertFrom-Json -AsHashtable } else { $null }
$payload = @{ ai = $ai; generated = (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'); documents = $documents; navigation = $navigation } | ConvertTo-Json -Depth 8 -Compress -EscapeHandling EscapeHtml
$template = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/index.html'))
$diagramScript = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/diagrams.js'))
$mermaidVendor = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/vendor/mermaid-11.12.0.min.js')).Replace('</script', '<\/script')
$workflowScript = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/task-records.js')) + "`n" + [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/workflow.js')) + "`n" + [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'wiki-viewer/test-feedback.js'))
$page = $template.Replace('__WX_WORKFLOW_SCRIPT__', $workflowScript).Replace('__WX_DIAGRAM_SCRIPT__', $diagramScript).Replace('__WX_MERMAID_VENDOR__', $mermaidVendor).Replace('__WX_WIKI_DATA__', $payload)
# 기존 파일 URL을 유지해 브라우저에 보존된 테스트 결과 입력을 잃지 않는다.
$output = Join-Path $repo 'Saved/Workflow/index.html'
New-Item -ItemType Directory -Path (Split-Path $output -Parent) -Force | Out-Null
[IO.File]::WriteAllText($output, $page, [Text.UTF8Encoding]::new($false))
Write-Output "Workflow viewer: $output ($($documents.Count) documents)"
if ($Open) { Start-Process -FilePath $output }
