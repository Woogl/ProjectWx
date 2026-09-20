# Copyright Woogle. All Rights Reserved.
[CmdletBinding()]
param(
    [string]$RepoRoot
)

# 공용 스킬은 .agents/skills, 훅과 보조 스크립트는 .agents/scripts에서 관리한다.
# Codex는 .agents/skills를 직접 사용하며, Claude Code용 .claude/skills는 이를 가리키는 junction이다.
# 클론 직후나 junction이 사라졌을 때 이 스크립트를 실행한다. 스킬 수정은 .agents/skills에서 한다.
# 하네스별 설정: Claude Code는 .claude/settings.json과 .mcp.json, Codex는 .codex/config.toml.
$ErrorActionPreference = 'Stop'
if (!$RepoRoot) { $RepoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent }
$repo = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
$source = Join-Path $repo '.agents\skills'
$link = Join-Path $repo '.claude\skills'

if (!(Test-Path $source)) { throw "정본 스킬 폴더가 없습니다: $source" }

# 깨진 junction 은 Test-Path 가 잡지 못하므로 Get-Item -Force 로 확인한다.
$item = Get-Item $link -Force -ErrorAction SilentlyContinue
if ($item) {
    if ($item.LinkType -ne 'Junction') {
        # 실물 폴더를 자동으로 지우면 옮기지 않은 작업물이 날아간다.
        throw "$link 이(가) junction 이 아닌 실제 폴더입니다. 내용을 .agents\skills 로 옮기고 지운 뒤 다시 실행하세요."
    }
    $target = @($item.Target)[0]
    if ($target -and [IO.Path]::GetFullPath($target).TrimEnd('\') -eq $source.TrimEnd('\')) {
        Write-Host "이미 연결되어 있습니다: $link -> $source"
        exit 0
    }
    Remove-Item $link -Force
}

$parent = Split-Path $link -Parent
if (!(Test-Path $parent)) { New-Item -ItemType Directory -Path $parent | Out-Null }
New-Item -ItemType Junction -Path $link -Target $source | Out-Null
Write-Host "연결했습니다: $link -> $source"
