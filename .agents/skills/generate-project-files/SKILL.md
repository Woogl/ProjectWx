---
name: generate-project-files
description: 프로젝트 루트의 BatchFiles/GenerateProjectFiles.bat을 실행해 Wx.sln 및 Visual Studio 프로젝트 파일을 재생성한다.
user-invocable: true
allowed-tools: PowerShell, Glob
---

# Visual Studio 프로젝트 파일 재생성

프로젝트 루트의 `BatchFiles/GenerateProjectFiles.bat`을 실행한다. 엔진 탐색과 UBT 인자는 배치 파일이 관리하므로 스킬에서 다시 구현하거나 우회하지 않는다.

## 절차

프로젝트 루트에서 아래 PowerShell을 실행한다.

```powershell
$ErrorActionPreference = 'Stop'

$projectRoot = (Get-Location).Path
$batchFile = Join-Path $projectRoot 'BatchFiles\GenerateProjectFiles.bat'
$solutionFile = Join-Path $projectRoot 'Wx.sln'

if (-not (Test-Path -LiteralPath $batchFile -PathType Leaf)) {
    throw "프로젝트 파일 생성 배치를 찾을 수 없습니다: $batchFile"
}

& $batchFile
$exitCode = $LASTEXITCODE

if ($exitCode -ne 0) {
    throw "프로젝트 파일 생성에 실패했습니다. 종료 코드: $exitCode"
}

if (-not (Test-Path -LiteralPath $solutionFile -PathType Leaf)) {
    throw "배치는 성공했지만 Wx.sln을 찾을 수 없습니다: $solutionFile"
}

"생성/확인된 솔루션: $solutionFile"
```

## 결과 보고

- 배치 종료 코드가 `0`이고 `Wx.sln`이 존재하면 성공으로 보고한다.
- 실패하면 배치 출력과 종료 코드를 함께 보고한다.
