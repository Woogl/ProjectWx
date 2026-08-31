---
name: build-doctor
description: UE 5.8 C++ 프로젝트의 Editor Development 빌드를 실행하고, 빌드 시작 전 환경 오류와 최초의 인과성 높은 컴파일 오류를 구분해 한국어로 진단한다.
---

# Build Doctor

UE 5.8 프로젝트의 `<프로젝트명>Editor` / Win64 / Development 빌드를 실행하고 실패 원인을 진단한다.

## 실행

프로젝트 루트에서 이 스킬의 `scripts/Invoke-WxEditorBuild.ps1`을 실행한다. 스크립트 경로는 현재 `SKILL.md` 위치를 기준으로 해석하고, `-ProjectRoot`에는 프로젝트 루트의 절대 경로를 전달한다.

Codex 데스크톱의 제한된 파일시스템 샌드박스에서는 첫 실행부터 승인된 샌드박스 외 실행을 요청한다. UnrealBuildTool이 `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`와 `Trace.uba`를 기록해야 하기 때문이다. 샌드박스 안에서 먼저 실패시킨 뒤 재시도하지 않는다. 권한이 없으면 스크립트의 쓰기 사전 검사가 UBT 실행 전에 종료 코드 `2`로 중단한다.

```powershell
& '<skill-root>\scripts\Invoke-WxEditorBuild.ps1' -ProjectRoot '<project-root>'
```

스크립트는 다음을 보장한다.

- 프로젝트 루트에서 단 하나의 `.uproject`를 찾는다.
- `%ProgramData%\Epic\UnrealEngineLauncher\LauncherInstalled.dat`에서 `UE_5.8` 설치 경로를 조회한다.
- 프로젝트 로그와 `%LOCALAPPDATA%\UnrealBuildTool`의 쓰기 가능 여부를 UBT 실행 전에 검사한다.
- 전체 출력과 종료 코드를 `<project-root>\Saved\Logs\BuildDoctor\`에 저장한다.
- `BUILD_DOCTOR_RESULT`로 성공, 빌드 실패, 사전 검사 실패를 구분하고 실제 종료 코드를 보존한다.
- 재실행 가능한 실제 `Build.bat` 명령을 출력한다.

스크립트를 우회해 Markdown 안에서 빌드 명령을 다시 작성하지 않는다. 실행기가 실패했다면 먼저 실행기 오류를 진단한다.

## 진단

1. 출력된 `BUILD_DOCTOR_LOG` 경로의 로그를 직접 읽는다.
2. `BUILD_DOCTOR_RESULT=preflight-failure`이면 프로젝트, UE 설치 정보, 로그 쓰기 권한 같은 빌드 시작 전 오류로 분류한다.
3. 그 밖의 실패에서는 가장 먼저 나타난 인과성 높은 오류를 찾는다. 뒤따르는 오류는 별개의 원인이라는 근거가 없으면 결과 노이즈로 취급한다.
4. 실패 분류가 필요할 때만 [references/common-ue5-build-failures.md](references/common-ue5-build-failures.md)를 읽는다.
5. 근거가 부족하면 최선 추정임을 밝히고 가능성이 높은 원인 2~3개만 우선순위대로 제시한다.

오류 우선순위는 다음과 같다.

1. 실행기 및 UnrealBuildTool 시작 전 오류
2. UnrealHeaderTool 및 생성 코드 오류
3. C/C++ 컴파일러 오류
4. include, 경로, 모듈 의존성 오류
5. 링커 오류
6. 타겟, 플러그인, 구성 불일치
7. 오래된 Intermediate 또는 Live Coding 산출물

## 보고 형식

항상 한국어로 다음 형식을 사용한다.

```markdown
## 빌드 결과
- 상태: 성공 | 실패 | 빌드 미시작
- 로그: <절대 경로>

## 원인 요약
- <1~3줄>

## 근거 로그
> <관련 로그 1>
> <관련 로그 2>
> <선택 로그 3>

## 수정 방법
1. <가장 확실한 조치>
2. <다음 조치>
3. <선택 검증 단계>

## 재실행 명령
<스크립트가 출력한 Build.bat 명령>
```

성공 시에는 원인과 수정 방법을 성공 근거와 추가 조치 불필요로 짧게 작성한다. 수십 줄의 오류를 인용하지 않는다.

## 변경 권한

진단 후 소스 코드나 설정을 자동으로 수정하지 않는다. 사용자가 수정을 요청하거나 제안한 수정안을 승인한 뒤에만 변경한다. 에디터의 DLL 점유가 원인이면 에디터 종료 후 재빌드 또는 `run-editor` 스킬을 안내하되 자동 실행하지 않는다.
