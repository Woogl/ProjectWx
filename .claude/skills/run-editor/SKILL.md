---
name: run-editor
description: 실행 중인 에디터/게임을 종료하고 WxEditor(Development) 타겟을 빌드한 뒤 언리얼 에디터로 프로젝트를 다시 실행한다.
user-invocable: true
allowed-tools: PowerShell, Glob
---

# 종료 → 빌드 → 실행

실행 중인 언리얼 에디터/게임을 종료해 DLL 잠금을 풀고, 에디터 타겟을 빌드한 뒤, 에디터로 프로젝트를 다시 연다.
(종료를 먼저 하는 이유: 에디터가 켜져 있으면 `UnrealEditor-WxGame.dll` 등이 잠겨 빌드가 실패한다.)

빌드 대상은 **`<프로젝트명>Editor` 타겟 / Win64 / Development** 이며, 실행은 **에디터 열기**다. (사용자가 직접 Play)

## 절차

### 1단계: 종료 → 빌드 → 실행 (한 번에)

아래 PowerShell 스크립트를 그대로 실행하라. 빌드가 실패(종료 코드 ≠ 0)하면 **에디터를 실행하지 않고** 멈춘다.

```powershell
$ErrorActionPreference = 'Stop'

# --- 0) 프로젝트 / 엔진 해석 ---
$uproject = Get-ChildItem -Path . -Filter *.uproject -File | Select-Object -First 1
if (-not $uproject) { throw '.uproject 파일을 찾을 수 없습니다. 프로젝트 루트에서 실행하세요.' }
$projPath = $uproject.FullName
$projName = [System.IO.Path]::GetFileNameWithoutExtension($projPath)   # 예: Wx
$editorTarget = "${projName}Editor"                                    # 예: WxEditor

$assoc = (Get-Content $projPath -Raw | ConvertFrom-Json).EngineAssociation
$engine = $null
$rk = "Registry::HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine\$assoc"
if (Test-Path $rk) { $engine = (Get-ItemProperty $rk).InstalledDirectory }
if (-not $engine) {
  $rkb = 'Registry::HKEY_CURRENT_USER\Software\Epic Games\Unreal Engine\Builds'
  if (Test-Path $rkb) { $v = (Get-ItemProperty $rkb).$assoc; if ($v) { $engine = $v } }   # 소스 빌드(GUID)
}
if (-not $engine -and $assoc -match '^[\d.]+$') { $engine = "C:\Program Files\Epic Games\UE_$assoc" }
if (-not $engine -or -not (Test-Path $engine)) { throw "엔진 경로를 찾을 수 없습니다 (EngineAssociation=$assoc)." }

$buildBat  = Join-Path $engine 'Engine\Build\BatchFiles\Build.bat'
$editorExe = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
foreach ($p in @($buildBat, $editorExe)) { if (-not (Test-Path $p)) { throw "필수 파일 없음: $p" } }
"프로젝트: $projPath"
"엔진: $engine"
"빌드 타겟: $editorTarget Win64 Development"

# --- 1) 실행 중인 에디터/게임 종료 (이 프로젝트만) ---
$targets = @()
$targets += Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%.exe'" -ErrorAction SilentlyContinue |   # DebugGame 등 구성별 바이너리(UnrealEditor-Win64-DebugGame.exe 등) 포함
  Where-Object { $_.CommandLine -and ($_.CommandLine -like "*$projPath*" -or $_.CommandLine -like "*$projName.uproject*") }
$targets += Get-CimInstance Win32_Process -Filter "Name='$projName.exe' OR Name LIKE '$projName-Win64-%.exe'" -ErrorAction SilentlyContinue   # 스탠드얼론/패키지 게임(구성별 접미사 포함)
$ids = $targets | Select-Object -ExpandProperty ProcessId -Unique
if ($ids) {
  $ids | ForEach-Object { Stop-Process -Id $_ -Force -ErrorAction SilentlyContinue }
  foreach ($id in $ids) { try { Wait-Process -Id $id -Timeout 30 -ErrorAction Stop } catch {} }   # 종료(잠금 해제) 대기
  "종료한 프로세스: $($ids -join ', ')"
} else {
  "실행 중인 이 프로젝트의 에디터/게임 없음 (종료 건너뜀)"
}

# --- 2) 빌드 ---
"=== 빌드 시작 ==="
& $buildBat $editorTarget Win64 Development "-Project=$projPath" -WaitMutex
$buildExit = $LASTEXITCODE
"=== 빌드 종료 코드: $buildExit ==="
if ($buildExit -ne 0) { throw "빌드 실패 (종료 코드 $buildExit). 에디터를 실행하지 않습니다. 위 로그의 에러를 확인하세요." }

# --- 3) 에디터 실행 ---
$proc = Start-Process -FilePath $editorExe -ArgumentList "`"$projPath`"" -PassThru
"에디터 실행 (PID $($proc.Id)): $editorExe `"$projPath`""
```

### 2단계: 결과 보고

- 종료한 프로세스(있었다면), 빌드 성공 여부, 에디터 실행 여부를 요약 보고한다.
- 빌드가 실패하면 에디터는 실행되지 않는다. 로그 끝부분의 컴파일 에러(파일·라인)를 함께 정리해 보고하고,
  필요하면 `build-doctor` 스킬로 진단을 이어가도록 안내한다.

## 참고

- 빌드 구성은 **Development Editor** 고정이다. DebugGame 등 다른 구성이 필요하면 스크립트의
  `Development` 인자를 바꾼다 (예: `DebugGame`).
- 종료 대상은 **현재 프로젝트의** 에디터/게임 프로세스만이다(명령줄에 `.uproject` 경로가 포함된 인스턴스).
  다른 프로젝트의 에디터는 건드리지 않는다.
- 빌드만 실패 없이 끝나면 에디터가 새 창으로 뜬다. 에디터는 대기하지 않고 백그라운드로 실행된다.
