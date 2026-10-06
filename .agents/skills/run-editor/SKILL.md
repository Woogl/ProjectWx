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

### 1단계: 사전 검사 → 종료 확인 → 빌드 → 실행

프로젝트 루트에서 아래 실행기를 호출한다. 경로는 현재 `SKILL.md`를 기준으로 해석하고, `-ProjectRoot`에는 실제 체크아웃의 절대 경로를 전달한다.

```powershell
& '<skill-root>\scripts\Invoke-WxEditor.ps1' -ProjectRoot '<project-root>'
```

- [build-doctor](../build-doctor/SKILL.md)의 실행 권한 지침을 따른다. UBT 사용자 로그 경로에 쓸 권한이 필요한 환경에서는 실행기 전체를 그 권한으로 실행한다.
- 공용 사전 검사로 프로젝트가 지정한 엔진, 에디터 실행 파일, 프로젝트·UBT 로그 쓰기 권한을 확인한 뒤 프로세스를 종료한다. 사전 검사가 실패하면 에디터를 종료하지 않는다.
- `Get-WxProjectProcess.ps1`로 현재 프로젝트임을 확인한 에디터(커맨드릿 포함)·게임만 선택한다. 상대경로나 정보 누락으로 소유 프로젝트가 모호하면 제외한다.
- `Stop-WxProjectProcesses.ps1`은 종료와 대기 실패를 확인하고 남은 프로젝트 프로세스를 다시 조회한다. 종료가 확인되지 않으면 빌드하지 않는다. 조회 직후 이미 종료된 프로세스는 정상으로 처리한다.
- 빌드는 `build-doctor/scripts/Invoke-WxEditorBuild.ps1`이 담당한다. 명령을 별도로 조립하지 않는다. 빌드 실패 시 실제 종료 코드를 보존하고 에디터를 실행하지 않는다.
- 빌드 성공 후 사용자가 조작할 에디터 창을 연다. 실행기 성공은 프로세스 시작을 뜻하며 프로젝트 로딩 완료까지 확인한 것은 아니다.

### 2단계: 결과 보고

- `RUN_EDITOR_STOPPED_PIDS`, `BUILD_DOCTOR_LOG`, `BUILD_DOCTOR_RESULT`, `RUN_EDITOR_STARTED_PID`와 실행기 종료 코드를 근거로 종료·빌드·실행 결과를 요약한다.
- 종료 실패를 종료 성공으로 보고하지 않는다. 빌드 실패는 공용 빌드 로그를 읽고 `build-doctor`의 진단 절차로 원인을 정리한다.
- 기본 구성은 Development Editor다. 다른 구성을 명시적으로 요청받으면 실행기와 공용 빌드 경로의 지원 여부를 먼저 확인한다.
