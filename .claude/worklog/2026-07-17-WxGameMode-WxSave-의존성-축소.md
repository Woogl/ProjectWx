# AWxGameMode 의 WxSave 의존성 축소

## 계획

### 목표

`AWxGameMode` 가 WxSave 의 데이터 모델을 너무 깊이 안다. 헤더 셋을 include 하고 `RespawnTransform` 의 Identity sentinel 규약, `TravelData.Map` 의 맵 키 표현, `bHasPlayerStats` 게이트를 직접 해석한다. 근본 원인은 WxSave 쪽에 있다 — `GetRespawnTransform()` 이 판정 없는 raw getter인 데다 호출자 없는 dead code라, 세이브가 "부활 지점 어디?" 에 완결된 답을 주지 못해 GameMode 가 판정을 떠안았다. 세이브가 완결된 질문에 답하게 만들어 GameMode 에는 스폰 정책만 남긴다. 동작 변경은 없다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/.../Public\|Private/WxSaveGameSubsystem.h/.cpp` | `GetRespawnTransform()` 제거 → `bool TryGetRespawnTransform(const UWorld*, FTransform&) const`(sentinel + 맵 일치 게이트 내장, World null 가드). `void ApplySavedPlayerStats(AActor*) const` 추가(`bHasPlayerStats` 게이트 + 기존 `UWxSaveWorldSubsystem::ApplyPlayerStats` 위임) | 수정 |
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | include 3→1(`WxSaveGameSubsystem.h`). `TryGetSavedRespawnTransform` 은 PIE 체크만 남기고 위임, `FinishRestartPlayer` 의 스탯 접근을 질의 한 줄로. 헤더 주석에서 sentinel·맵 일치 설명 제거 | 수정 |

`GetStableMapPackageName`·`GetSaveGame()` 은 WxSave 내부(`FlushMapTravelData` 등)가 쓰므로 public 유지.

### 접근 방식

- **GameMode 가 던지는 질문을 그대로 API 로 만든다**: GameMode 의 관심사는 "어디에 스폰하나"·"스탯 복원해" 둘뿐인데 그 답을 얻으려 세이브 내부를 뒤지고 있다. 판정을 세이브 안으로 옮기면 질문만 남고, 저장 포맷 변경이 GameMode 로 번지지 않는다.

- **PIE "여기서 플레이" 우선 규칙은 GameMode 에 남긴다**: 세이브 도메인 지식이 아니라 스폰 정책(엔진 `ChoosePlayerStart` 의 최우선 규칙과 짝)이라 세이브가 알 이유가 없다.

- **서브시스템 획득은 풀어쓴다**: 두 곳에서 반복되지만 헬퍼로 묶지 않는다(기존 코드 형태 유지).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/.../Public/WxSaveGameSubsystem.h` | `GetRespawnTransform()` → `TryGetRespawnTransform(const UWorld*, FTransform&)`, `ApplySavedPlayerStats(AActor*)` 추가. `class AActor;` 전방 선언 | 수정 |
| `Plugins/WxSave/.../Private/WxSaveGameSubsystem.cpp` | 두 함수 구현 — sentinel·맵 일치 게이트와 `bHasPlayerStats` 게이트를 GameMode 에서 이관 | 수정 |
| `Source/WxGame/Framework/WxGameMode.cpp` | include 3→1. `TryGetSavedRespawnTransform` 은 PIE 체크 후 위임, `FinishRestartPlayer` 의 스탯 접근은 질의 한 줄로 | 수정 |
| `Source/WxGame/Framework/WxGameMode.h` | `TryGetSavedRespawnTransform` 주석에서 sentinel·맵 일치 설명 제거(그 지식이 WxSave 로 감) | 수정 |

### 구현·결정과 그 이유

- **dead code 가 결합의 원인이었다**: `GetRespawnTransform()` 은 호출자가 없었고, 판정 없는 raw getter라 쓸 수도 없었다. GameMode 가 그걸 우회해 SaveGame 을 직접 뜯으면서 데이터 모델 전체를 알게 된 구조다. 게터를 지우고 완결된 질의로 바꾸니 결합이 따라서 풀렸다.

- **PIE 체크만 GameMode 에 남김**: `APlayerStartPIE` 우선 규칙은 세이브 도메인 지식이 아니라 스폰 정책이라, 세이브가 알면 오히려 역방향 결합이 된다. GameMode 는 PIE 일 때 세이브에 묻지도 않는다.

- **`World` null 가드 추가**: 기존 GameMode 는 `GetStableMapPackageName(GetWorld())` 를 null 체크 없이 불렀다(내부에서 `World->GetOutermost()` 역참조). 판정을 옮기면서 가드를 넣었다 — 동작 차이가 아니라 기존 잠재 크래시 경로를 닫은 것이다.

- **서브시스템 획득은 두 곳에서 풀어씀**: 반복이지만 헬퍼로 묶지 않았다(기존 코드 형태 유지).

### 계획 대비 달라진 점
- 계획대로. `class AActor;` 전방 선언 추가는 계획에 없었으나 `AActor*` 인자에 필요했다.

### 후속 과제
- **런타임 동작 미검증**: 빌드는 통과했고 판정식은 옮긴 것이라 정적으로는 등가지만, 실행 경로가 바뀌었으므로 에디터 확인이 남았다 — 체크포인트 접촉 → 저장 → 사망/로드로 그 지점에 스탯과 함께 부활하는지, PIE "여기서 플레이" 가 여전히 저장 위치를 무시하는지.
