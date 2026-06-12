---
name: clean-stale-snapshots
description: 원본 .uasset이 삭제/리네임되어 더 이상 존재하지 않는 스테일 Blueprint 스냅샷 JSON을 찾아 삭제한다.
user-invocable: true
allowed-tools: PowerShell
---

# 스테일 Blueprint 스냅샷 정리

`Plugins/WxBlueprintSnapshot/Snapshots/` 아래의 스냅샷 JSON은 원본 BP 애셋을 미러링한다.
원본 `.uasset`이 삭제되거나 리네임되면 스냅샷 JSON만 남아 스테일 상태가 되므로, 이를 찾아 삭제하라.

## 매핑 규칙

스냅샷 경로의 최상위 폴더는 콘텐츠 마운트 루트를 의미한다.

| 스냅샷 경로 | 원본 애셋 경로 |
| --- | --- |
| `Snapshots/Game/<상대경로>.json` | `Content/<상대경로>.uasset` |
| `Snapshots/<플러그인명>/<상대경로>.json` | `Plugins/**/<플러그인명>/Content/<상대경로>.uasset` |

플러그인 루트는 `Plugins/` 아래에서 `<플러그인명>.uplugin` 파일을 찾아 해석한다.

## 절차

### 1단계: 정리 실행

아래 PowerShell 스크립트를 프로젝트 루트에서 그대로 실행하라.
마운트 루트를 해석할 수 없는 폴더는 삭제하지 않고 경고만 남긴다(오삭제 방지).

```powershell
$ErrorActionPreference = 'Stop'
$projectRoot = (Get-Location).Path
$snapRoot = Join-Path $projectRoot 'Plugins\WxBlueprintSnapshot\Snapshots'
if (-not (Test-Path $snapRoot)) { throw "스냅샷 폴더를 찾을 수 없습니다: $snapRoot" }

$deleted = @()
$skipped = @()
foreach ($rootDir in Get-ChildItem $snapRoot -Directory) {
  if ($rootDir.Name -eq 'Game') {
    $contentRoot = Join-Path $projectRoot 'Content'
  } else {
    $uplugin = Get-ChildItem -Path (Join-Path $projectRoot 'Plugins') -Recurse -Filter "$($rootDir.Name).uplugin" -File | Select-Object -First 1
    if (-not $uplugin) { $skipped += $rootDir.Name; continue }
    $contentRoot = Join-Path $uplugin.DirectoryName 'Content'
  }
  foreach ($json in Get-ChildItem $rootDir.FullName -Recurse -Filter *.json -File) {
    $rel = $json.FullName.Substring($rootDir.FullName.Length).TrimStart('\')
    $asset = Join-Path $contentRoot ($rel -replace '\.json$', '.uasset')
    if (-not (Test-Path $asset)) {
      Remove-Item $json.FullName -Confirm:$false
      $deleted += $json.FullName.Substring($snapRoot.Length + 1)
    }
  }
}

Get-ChildItem $snapRoot -Recurse -Directory |
  Sort-Object { $_.FullName.Length } -Descending |
  Where-Object { -not (Get-ChildItem $_.FullName -Recurse -File) } |
  ForEach-Object { Remove-Item $_.FullName -Confirm:$false }

"삭제: $($deleted.Count)건"
$deleted | ForEach-Object { " - $_" }
if ($skipped) { "경고: 마운트 루트 해석 실패로 건너뜀: $($skipped -join ', ')" }
```

### 2단계: 결과 보고

- 삭제된 파일 목록과 개수를 보고한다. 삭제가 없으면 "스테일 스냅샷 없음"으로 보고한다.
- 마운트 루트 해석에 실패해 건너뛴 폴더가 있으면 반드시 함께 보고한다.
- 이 스킬은 git 커밋/푸시를 수행하지 않는다. 호출자가 필요 시 별도로 처리한다.
