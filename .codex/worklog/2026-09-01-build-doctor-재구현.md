# build-doctor 재구현

## 계획

- 반복되는 빌드 절차를 `Invoke-WxEditorBuild.ps1` 스크립트로 분리한다.
- 빌드 로그를 프로젝트의 쓰기 가능한 `Saved/Logs/BuildDoctor` 아래에 고유한 이름으로 저장한다.
- 프로젝트 파일, UE 5.8 런처 설치 정보, `Build.bat`, 로그 디렉터리 쓰기 가능 여부를 빌드 전에 검증한다.
- 전체 빌드 출력과 실제 종료 코드를 로그 및 호출자에게 보존한다.
- `SKILL.md`는 스크립트 실행, 최초 원인 진단, 보고 및 수정 승인 규칙 중심으로 간소화한다.
- 기존 스킬 내부 로그와 무시 규칙은 보존하고, 신규 로그 위치를 분리하며 공통 실패 참조에 빌드 인프라 오류를 보완한다.
- 스킬 검증기와 스크립트를 통한 UE 5.8 `WxEditor Win64 Development` 실제 빌드로 검증한다.

## 완료

- 빌드 실행을 `scripts/Invoke-WxEditorBuild.ps1`로 분리하고 프로젝트, UE 5.8 런처 설치 정보, 프로젝트 로그 및 UBT 사용자 로컬 데이터 경로를 사전 검증하도록 구현했다.
- 신규 로그를 `Saved/Logs/BuildDoctor`에 고유한 이름으로 저장하고, 재실행 명령·결과 종류·실제 종료 코드를 로그와 표준 출력에 함께 남기도록 했다.
- 제한된 샌드박스에서는 UBT를 시작하기 전에 `BUILD_DOCTOR_RESULT=preflight-failure`, 종료 코드 2로 중단하고, 승인된 샌드박스 외 실행에서는 UBT 사용자 로컬 로그 기록을 허용하도록 스킬 지침을 보완했다.
- 공통 오류 참조에 로그 권한, CLR 예외 코드 `-532462766`, UBT 시작 실패 및 에디터 DLL 점유 분류를 추가했다.
- 기존 `.agents/skills/build-doctor/logs`의 과거 로그와 `.gitignore`는 보존했다.
- PowerShell 파서 오류 0건, 스킬 frontmatter·이름·설명·스크립트·참조 연결 수동 검사 통과, 제한 실행 사전 실패 경로 검증을 완료했다.
- 최종 UE 5.8 `WxEditor Win64 Development` 빌드는 성공했고 로그는 `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-01_023735_769_21832.log`에 저장됐다.
- `quick_validate.py`는 번들 Python에 `PyYAML`이 없어 실행되지 않았으며, 외부 패키지를 설치하지 않고 위의 구조 검사와 실제 동작 검증으로 대체했다.
