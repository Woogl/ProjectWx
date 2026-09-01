# generate-project-files 스킬 갱신

## 계획

- `generate-project-files/SKILL.md`에서 레지스트리 및 `UnrealVersionSelector` 실행 로직을 제거한다.
- 프로젝트 루트의 `BatchFiles/GenerateProjectFiles.bat` 존재 여부를 확인한 뒤 해당 파일만 실행하도록 변경한다.
- 배치 종료 코드가 `0`이고 `Wx.sln`이 존재할 때만 성공으로 보고하도록 유지한다.
- `quick_validate.py`로 스킬 형식을 검증하고, 실제 스킬 절차대로 배치 파일을 실행해 확인한다.

## 완료

- `generate-project-files/SKILL.md`에서 레지스트리 및 `UnrealVersionSelector` 실행 로직을 제거했다.
- 프로젝트 루트의 `BatchFiles/GenerateProjectFiles.bat`만 실행하고, 배치 종료 코드와 `Wx.sln` 존재 여부를 검사하도록 변경했다.
- `quick_validate.py` 실행을 시도했으나 제공된 Python 환경에 `PyYAML` 모듈이 없어 검증기 자체가 시작되지 않았다.
- 프런트매터, 스킬 이름, 설명, 미완성 자리표시자, 배치 경로 포함 여부 및 레거시 실행 경로 제거 여부를 직접 검사해 통과했다.
- 변경된 스킬에 기재한 PowerShell 절차를 그대로 실행해 UBT `Result: Succeeded`와 `Wx.sln` 생성을 확인했다.
