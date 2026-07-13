# 체크포인트 IsDefaultStart 및 시작지점 선택 인프라 제거

## 계획

### 목표
좌표(RespawnTransform) 기반 부활 리팩터 이후 `bIsDefaultStart`/`IsDefaultStart()` 는 "세이브 없는 최초 접속" 시작지점 선택 용도만 남았다. 이를 제거하고 신규 세션 시작지점을 일반 `APlayerStart`(엔진 기본 선택)로 맡긴다. 그 결과 no-op 가 되는 `UWxPlayerSpawningComponent` 와 `WxGameMode` 위임 오버라이드도 함께 제거한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/WorldObject/WxCheckPoint.h` | `IsDefaultStart()`·`bIsDefaultStart`·`#if WITH_EDITOR`(PostDuplicate/CheckForErrors) 제거, doc 주석 정리 | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 위 정의들 + 미사용 `#if WITH_EDITOR` 인클루드 블록 제거 | 수정 |
| `Source/WxGame/Framework/WxPlayerSpawningComponent.h` | 파일 삭제 | 삭제 |
| `Source/WxGame/Framework/WxPlayerSpawningComponent.cpp` | 파일 삭제 | 삭제 |
| `Source/WxGame/Framework/WxGameMode.h` | `ChoosePlayerStart_Implementation` 오버라이드 제거, 클래스 doc 주석 정리 | 수정 |
| `Source/WxGame/Framework/WxGameMode.cpp` | 위 정의 제거, 미사용 인클루드(WxPlayerSpawningComponent.h/WxGameState.h) 제거 | 수정 |
| `Source/WxGame/Framework/WxGameState.h` | doc 주석에서 "PlayerSpawning" 언급 제거 | 수정 |
| `Source/WxGame/README.md` | 삭제된 클래스/플래그 언급 정리 | 수정 |

### 접근 방식
- **시작지점 = 일반 APlayerStart**: 체크포인트는 순수 부활 지점(RespawnTransform)이 되고, 신규 세션 시작지점은 엔진 기본 `ChoosePlayerStart`(PIE + 미점유 APlayerStart)가 처리한다.
- **컴포넌트 통째 제거**: `ChoosePlayerStart` 의 체크포인트 순회를 빼면 남는 로직이 엔진 기본과 중복되는 no-op 라, 컴포넌트와 GameMode 위임을 함께 제거해 죽은 코드를 남기지 않는다.
- **오버라이드 제거는 파생 정리 유도**: `bIsDefaultStart` 가 빠지면 `PostDuplicate`/`CheckForErrors` 는 본문이 비므로 오버라이드째 삭제(베이스 `AWxGimmick` 가 WxSaveId 재부여 담당). `ChoosePlayerStart_Implementation` 은 순수 Super 패스스루가 되어 오버라이드 불필요.

### 디자이너 후속(비치명적)
- `GM_Combat` 의 `FrameworkComponents` 에서 `WxPlayerSpawningComponent` 항목 제거(InitGame 이 null 방어하므로 빌드/실행 무영향).
- 레벨에 신규 세션용 일반 `APlayerStart` 배치 확인.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/WorldObject/WxCheckPoint.h` | `IsDefaultStart()`·`bIsDefaultStart`·`#if WITH_EDITOR`(PostDuplicate/CheckForErrors) 제거, doc 주석 정리 | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 위 정의 + 미사용 `#if WITH_EDITOR` 인클루드 블록 제거 | 수정 |
| `Source/WxGame/Framework/WxPlayerSpawningComponent.h/.cpp` | `git rm -f` 로 삭제 | 삭제 |
| `Source/WxGame/Framework/WxGameMode.h` | `ChoosePlayerStart_Implementation` 오버라이드 제거, 클래스 doc 주석 정리 | 수정 |
| `Source/WxGame/Framework/WxGameMode.cpp` | 위 정의 + 미사용 인클루드(WxPlayerSpawningComponent.h/WxGameState.h) 제거 | 수정 |
| `Source/WxGame/Framework/WxGameState.h` | doc 주석에서 "PlayerSpawning" 언급 제거 | 수정 |
| `Source/WxGame/README.md` | 삭제된 클래스/플래그 언급 정리, 시작지점 규약을 일반 APlayerStart 로 갱신 | 수정 |

### 구현·결정과 그 이유
- **시작지점을 엔진 기본에 위임**: 체크포인트가 `AWxGimmick`(APlayerStart 아님)이 된 이상 시작지점 후보가 될 수 없어, 신규 세션 시작지점을 레벨의 일반 `APlayerStart`로 넘겼다. 부활은 이미 `RespawnTransform`이 전담하므로 역할이 깔끔히 분리된다.
- **컴포넌트·오버라이드 통째 제거**: 체크포인트 순회를 빼면 `ChoosePlayerStart`가 엔진 기본과 중복되는 no-op 라, 죽은 코드를 남기지 않으려 `UWxPlayerSpawningComponent`와 `WxGameMode::ChoosePlayerStart_Implementation`(순수 Super 패스스루가 됨)을 함께 삭제했다.
- **파생 오버라이드 삭제**: `bIsDefaultStart` 제거로 `PostDuplicate`/`CheckForErrors` 본문이 비어, 오버라이드째 제거했다. WxSaveId 재부여는 베이스 `AWxGimmick::PostDuplicate` 가 계속 담당하므로 회귀 없음.

### 계획 대비 달라진 점
- 계획대로. (빌드: WxEditor Development 성공 — UBT가 삭제 소스 감지 후 WxGame 모듈 클린 재컴파일·링크)

### 후속 과제
- **디자이너(비치명적)**: `GM_Combat` 의 `FrameworkComponents` 에서 `WxPlayerSpawningComponent` 항목 제거(InitGame null 방어로 빌드/실행 무영향). 레벨에 신규 세션용 일반 `APlayerStart` 배치 확인.
- **미검증(사용자)**: 슬롯 삭제 후 최초 접속 스폰, 체크포인트 상호작용→저장→부활, Map Check 무에러 확인.
