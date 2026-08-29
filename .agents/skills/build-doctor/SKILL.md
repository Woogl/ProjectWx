---
name: build-doctor
description: 프로젝트 빌드 에러를 진단하고 해결법을 제시합니다. 사용자가 원한다면 로컬에서 임시 수정합니다.
---

Build UE5 C++ projects and explain failures in Korean.

## Workflow

1. Build the editor target with the command in **Build command** below.
2. Save the full build output to a timestamped log under `\.Codex\skills\build-doctor\logs\` (e.g. `build_2026-06-07_231200.log`).
3. If the build fails, inspect the log directly and identify the earliest high-signal failure first.
4. Produce a short Korean report with:
   - 1-3 line cause summary
   - quoted log lines
   - immediate fixes to try now

## Build command

Build target is **`<프로젝트명>Editor` / Win64 / Development** (C++ 이터레이션 기준). Run the PowerShell below from the project root. 엔진 버전은 **UE 5.8 고정**이고 설치 경로만 런처 정보에서 조회한다(C 드라이브가 아닐 수 있으므로 경로는 박아두지 않는다). 스크립트는 빌드 후 전체 출력을 로그 파일에 tee하고 종료 코드를 출력한다(0 = 성공). Preserve the `Build.bat ...` command in the response so the user can rerun it.

```powershell
$ErrorActionPreference = 'Stop'

# --- 프로젝트 / 엔진 (UE 5.8 고정) ---
$uproject = Get-ChildItem -Path . -Filter *.uproject -File | Select-Object -First 1
if (-not $uproject) { throw '.uproject 파일을 찾을 수 없습니다. 프로젝트 루트에서 실행하세요.' }
$projPath = $uproject.FullName
$projName = [System.IO.Path]::GetFileNameWithoutExtension($projPath)   # 예: Wx
$editorTarget = "${projName}Editor"                                    # 예: WxEditor

# 버전은 5.8 고정, 설치 경로만 조회한다(설치 드라이브가 C가 아닐 수 있음).
$engine = $null
$dat = "$env:ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat"
if (Test-Path $dat) {
  $entry = (Get-Content $dat -Raw | ConvertFrom-Json).InstallationList | Where-Object { $_.AppName -eq 'UE_5.8' } | Select-Object -First 1
  if ($entry) { $engine = $entry.InstallLocation }
}
if (-not $engine) {
  $rk = 'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine\5.8'
  if (Test-Path $rk) { $engine = (Get-ItemProperty $rk).InstalledDirectory }   # 이 키는 없을 수 있다
}
if (-not $engine) { $engine = 'C:\Program Files\Epic Games\UE_5.8' }           # 최후 기본 경로

$buildBat = Join-Path $engine 'Engine\Build\BatchFiles\Build.bat'
if (-not (Test-Path $buildBat)) { throw "UE 5.8 Build.bat 없음: $buildBat" }

# --- 빌드 (전체 출력 로그 저장: 날짜·시간별) ---
$logDir = 'C:\Wx\.Codex\skills\build-doctor\logs'
if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir -Force | Out-Null }
$log = Join-Path $logDir ("build_{0}.log" -f (Get-Date -Format 'yyyy-MM-dd_HHmmss'))
"빌드 타겟: $editorTarget Win64 Development"
"로그: $log"
& $buildBat $editorTarget Win64 Development "-Project=$projPath" -WaitMutex 2>&1 | Tee-Object -FilePath $log
"=== EXIT CODE: $LASTEXITCODE ==="
```

Notes:
- 에디터가 켜져 있으면 `UnrealEditor-*.dll` 잠금으로 **링크 단계(LNK)에서 실패**한다. 링크 에러가 보이면 에디터를 닫고 재빌드하거나 `run-editor` 스킬(종료→빌드→실행)을 안내한다.
- `BuildProjectFiles.bat`은 **프로젝트 파일 재생성**용이지 빌드 명령이 아니다. 빌드에는 위 `Build.bat`을 쓴다.
- 다른 구성이 필요하면 `Development`를 `DebugGame` 등으로 바꾼다.

## Diagnosis rules

Always distinguish between the **first causal error** and downstream noise.

Prioritize errors in this order:
1. `UnrealHeaderTool` / generated code failures
2. C/C++ compiler errors (`error Cxxxx`, `fatal error Cxxxx`, syntax/type issues)
3. include/path/module definition errors (`cannot open include file`, missing module dependency)
4. linker errors (`LNK2001`, `LNK2019`, `LNK1120`)
5. target/plugin/configuration mismatches
6. stale intermediates/hot reload artifacts

Use `references/common-ue5-build-failures.md` for mapping signatures to likely causes.

Important:
- Do not summarize every error line.
- Find the **earliest high-signal failure** and treat later errors as consequences unless the log clearly shows multiple unrelated failures.
- Prefer concrete causes like "`Build.cs` missing dependency on `GameplayTags`" over vague causes like "module problem".
- If the evidence is insufficient, say it is a best-effort diagnosis and list 2-3 plausible causes in priority order.

## Required output format

Always answer in Korean using this structure:

```markdown
## 빌드 결과
- 상태: 성공 | 실패

## 원인 요약
- <1-3줄 요약>

## 근거 로그
> <most relevant log line 1>
> <most relevant log line 2>
> <optional line 3>

## 수정 방법
1. <highest-confidence action>
2. <next action>
3. <optional validation step>
```

## Code modification rules

- After diagnosis, **do not modify code automatically**. Present the suggested fix and wait for user confirmation before applying any changes.

## What not to do

- Do not bury the answer in generic Unreal advice.
- Do not quote dozens of lines when 2-3 lines are enough.
- Do not ignore the first causal error in favor of the final summary line.
