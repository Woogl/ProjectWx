---
name: generate-project-files
description: .uproject에 우클릭 > Generate Visual Studio project files 를 실행해 .sln 및 VS 프로젝트 파일을 재생성한다.
user-invocable: true
allowed-tools: PowerShell, Glob
---

# Visual Studio 프로젝트 파일 재생성

`.uproject` 파일을 우클릭했을 때 나오는 **Generate Visual Studio project files** 와 동일한 동작을 수행하라.
이 메뉴는 레지스트리(`HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\rungenproj\command`)에 등록된
`UnrealVersionSelector.exe /projectfiles "<프로젝트경로>"` 명령으로 구현되어 있다. 이 스킬은 그 명령을 그대로 실행한다.

## 절차

### 1단계: 실행

아래 PowerShell 스크립트를 그대로 실행하라. 스크립트는 다음을 수행한다.

1. 작업 디렉터리 루트에서 `*.uproject`를 찾는다. (여러 개면 첫 번째를 사용하되, 그 사실을 보고에 남긴다.)
2. 레지스트리에서 우클릭 메뉴의 실제 명령(`rungenproj` → 없으면 `generateprojectfiles`)을 읽어 `%1`을 프로젝트 절대경로로 치환해 실행한다.
3. 레지스트리 항목이 없으면 알려진 경로의 `UnrealVersionSelector.exe /projectfiles`로 폴백한다.
4. 완료를 기다린 뒤 종료 코드와 `.sln` 갱신 결과를 출력한다.

```powershell
$ErrorActionPreference = 'Stop'

# 1) .uproject 찾기
$uproject = Get-ChildItem -Path . -Filter *.uproject -File | Select-Object -First 1
if (-not $uproject) { throw '.uproject 파일을 찾을 수 없습니다. 프로젝트 루트에서 실행하세요.' }
$projPath = $uproject.FullName
$count = (Get-ChildItem -Path . -Filter *.uproject -File | Measure-Object).Count
"프로젝트: $projPath" + $(if ($count -gt 1) { " (.uproject $count개 중 첫 번째 사용)" })

# 2) 우클릭 메뉴 명령을 레지스트리에서 해석
$cmdKeys = @(
  'Registry::HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\rungenproj\command',
  'Registry::HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\generateprojectfiles\command'
)
$rawCmd = $null
foreach ($k in $cmdKeys) {
  if (Test-Path $k) { $rawCmd = (Get-ItemProperty $k).'(default)'; break }
}

if ($rawCmd) {
  if ($rawCmd -notmatch '^\s*"([^"]+)"\s*(.*)$') { throw "레지스트리 명령 파싱 실패: $rawCmd" }
  $exe = $Matches[1]
  $argTemplate = $Matches[2]
  "명령 출처: 레지스트리"
} else {
  $exe = 'C:\Program Files\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe'
  $argTemplate = '/projectfiles "%1"'
  "명령 출처: 폴백(레지스트리 항목 없음)"
}
if (-not (Test-Path $exe)) { throw "UnrealVersionSelector를 찾을 수 없습니다: $exe" }

# 3) %1 치환 후 실행 (우클릭과 동일)
$argLine = $argTemplate -replace '%1', $projPath
"실행: `"$exe`" $argLine"
$slnBefore = Get-ChildItem -Path . -Filter *.sln -File | ForEach-Object { $_.LastWriteTime }
$proc = Start-Process -FilePath $exe -ArgumentList $argLine -Wait -PassThru

# 4) 결과
"종료 코드: $($proc.ExitCode)"
$sln = Get-ChildItem -Path . -Filter *.sln -File | Select-Object -First 1
if ($sln) {
  "생성/갱신된 솔루션: $($sln.FullName) (수정 시각 $($sln.LastWriteTime))"
} else {
  "경고: .sln 파일을 찾지 못했습니다. 종료 코드를 확인하세요."
}
```

### 2단계: 결과 보고

- 종료 코드가 `0`이고 `.sln`이 갱신되었으면 성공으로 보고한다.
- 종료 코드가 `0`이 아니면 실패로 보고한다. `UnrealVersionSelector`는 실패 시 GUI 오류 대화상자를 띄워
  대기 상태로 멈출 수 있으므로, 응답이 오래 걸리면 화면의 대화상자를 확인하라고 안내한다.
- `.uproject`가 여러 개라 첫 번째를 사용했다면 그 사실을 함께 밝힌다.
