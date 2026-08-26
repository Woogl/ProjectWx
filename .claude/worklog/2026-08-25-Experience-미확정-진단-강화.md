# Experience 미확정 진단 강화 + 없는 폴백 서술 정정

## 계획

### 목표
`ResolveExperienceId` 는 진입 URL → WorldSettings 2단계만 보는데, 주석·README 는 존재하지 않는 세 번째 폴백(`DefaultExperience`)을 약속하고 있다. 확정 실패 시에도 `Warning` 한 줄만 남아 "폰 없는 화면"의 원인이 드러나지 않는다. 폴백을 새로 만들지 않고, 서술을 실제 2단계에 맞추고 실패 로그를 `Error` 로 올려 원인·결과·조치를 지목하게 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxExperienceManagerComponent.cpp` | 무효 ID 분기 로그를 `Warning` → `Error`, 문구에 결과(폰 스폰 안 됨)와 조치(WorldSettings·URL 지정) 포함 | 수정 |
| `Source/WxGame/Framework/WxGameMode.h` | `ResolveExperienceId` 주석의 3단계 서술을 2단계 + 폴백 없음으로 정정 | 수정 |
| `Source/WxGame/Framework/WxGameMode.cpp` | `GetDefaultPawnClassForController` 의 "이미 경고한 상태다" 주석을 에러 기준으로 정정 | 수정 |
| `Source/WxGame/Framework/WxWorldSettings.h` | 클래스·함수 주석의 "자체 폴백 이전"·"다음 폴백" 서술 제거 | 수정 |
| `Source/WxGame/README.md` | 확정 우선순위 표·목록의 "폴백" 서술 정정 | 수정 |

### 접근 방식
- **폴백 신설 대신 서술 정정**: 개발용 기본 Experience 도입은 제외 결정. 코드가 하는 일(2단계)에 문서를 맞춘다.
- **단일 에러 지점**: `SetCurrentExperience` 의 유일한 호출자가 `AWxGameMode::InitGameState` 라, GameMode 에 로그를 더 두면 같은 원인에 에러가 두 줄이 된다. 확정 실패 진단은 기존 한 지점에 몰아 문구만 강화한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxExperienceManagerComponent.cpp` | 무효 ID 분기 로그 `Warning` → `Error`, 문구에 결과(프레임워크 주입·폰 스폰 중단)와 조치(WorldSettings 지정 / `?Experience=`) 포함 | 수정 |
| `Source/WxGame/Framework/WxGameMode.h` | `ResolveExperienceId` 주석을 URL → WorldSettings 2단계 + "그 뒤 폴백 없음"으로 정정 | 수정 |
| `Source/WxGame/Framework/WxGameMode.cpp` | `GetDefaultPawnClassForController` 조기 반환 주석을 에러 로그 기준으로 정정 | 수정 |
| `Source/WxGame/Framework/WxWorldSettings.h` | 클래스 주석은 "확정의 마지막 단계", 함수 주석은 "미지정 시 그대로 미확정"으로 정정 | 수정 |
| `Source/WxGame/README.md` | 핵심 타입 표와 확정 우선순위 항목의 "폴백" 서술 제거·정정 | 수정 |

### 구현·결정과 그 이유
- **폴백을 만들지 않았다**: `DefaultExperience` 는 이미 제거된 개념이라 되살리지 않고, 코드가 실제로 하는 2단계에 문서를 맞췄다. 확정 출처가 URL·맵 둘로 고정돼 폰 클래스 출처가 흐려지지 않는다.
- **에러를 한 지점에만 뒀다**: `SetCurrentExperience` 의 유일한 호출자가 `AWxGameMode::InitGameState` 라, GameMode 에 로그를 더하면 같은 원인에 에러가 두 줄 찍힌다. 대신 기존 한 줄이 원인·결과·조치를 모두 담게 문구를 늘렸다.
- **로그 문구가 URL·WorldSettings 를 언급한다**: 매니저가 확정 출처를 알 필요는 없지만 문자열일 뿐이고 둘 다 같은 `WxGame` 모듈이다. 로그를 본 사람이 바로 고칠 수 있는 편이 이득이 크다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음.
