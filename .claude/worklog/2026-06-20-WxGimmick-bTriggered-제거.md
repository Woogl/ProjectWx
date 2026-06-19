# AWxGimmick bTriggered 제거 — 1회성 기믹 자체 State enum 이전

## 계획

### 목표
base `AWxGimmick` 의 레거시 1회성 발동 플래그 `bTriggered`(Replicated + SaveGame)를 제거한다. 이를 쓰던 4개 소비자를 Door/Elevator 와 동일한 자체 2-State enum(gimmick-state-enum-pattern)으로 이전하고, base 에서 `bTriggered`/`MarkTriggered`/`OnRep_bTriggered`/복제 선언을 완전히 삭제한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxTreasureChest.{h,cpp}` | `EWxChestState{Closed,Open}` + State(ReplicatedUsing=OnRep_State,SaveGame)·GetLifetimeReplicatedProps·OnRep_State·`SetChestState` 추가, bTriggered/MarkTriggered 치환 | 수정 |
| `WxSpawnConsole.{h,cpp}` | `EWxSpawnConsoleState{Idle,Spawned}` 동일 구조 + `SetSpawnConsoleState` | 수정 |
| `WxAlarmConsole.{h,cpp}` | `EWxAlarmConsoleState{Idle,Alarmed}` 동일 구조 + `SetAlarmConsoleState` | 수정 |
| `WxLaserCorridor.{h,cpp}` (WxGame) | `EWxLaserCorridorState{Active,Disabled}` 동일 구조 + `SetLaserCorridorState` | 수정 |
| `WxGimmick.{h,cpp}` | bTriggered/MarkTriggered/OnRep_bTriggered/GetLifetimeReplicatedProps·`Net/UnrealNetwork.h` 삭제, doc 갱신 | 수정 |
| `WxDoor.h` | "base bTriggered" 문구 손질 | 수정 |
| 메모리 `gimmick-state-enum-pattern.md` | bTriggered 제거 완료·소비자 구분 갱신 | 수정 |

### 접근 방식
- **Elevator 패턴 채택**: 4개 소비자는 StateTree 바인딩이 없고 C++ `ApplyState` 가 상태를 소비하므로, Door(폴링)가 아니라 Elevator 패턴을 따른다 — `ReplicatedUsing=OnRep_State`(클라 OnRep→ApplyState), 권위 setter 가 `if(!HasAuthority()||State==NewState) return; State=NewState; ApplyState();`.
- **동작 보존**: 기존 `if (bTriggered)` → `if (State == 발동값)`, `MarkTriggered()` → `SetXxxState(발동값)` 로 1:1 치환. 발동 부수효과(애니/스폰/FX/타이머)와 복제·SaveGame 영속은 그대로 유지.
- **base 슬림화**: bTriggered 가 사라지면 base 는 복제 prop 이 없어 `GetLifetimeReplicatedProps` 오버라이드 자체를 제거. 자식의 `Super::` 호출은 `AActor` 버전으로 정상 귀결.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxTreasureChest.{h,cpp}` | `EWxChestState{Closed,Open}` + State·GetLifetimeReplicatedProps·OnRep_State·`SetChestState` 추가, `bTriggered`/`MarkTriggered` 치환 | 수정 |
| `WxSpawnConsole.{h,cpp}` | `EWxSpawnConsoleState{Idle,Spawned}` 동일 구조 + `SetSpawnConsoleState` | 수정 |
| `WxAlarmConsole.{h,cpp}` | `EWxAlarmConsoleState{Idle,Alarmed}` 동일 구조 + `SetAlarmConsoleState` | 수정 |
| `WxLaserCorridor.{h,cpp}` (WxGame) | `EWxLaserCorridorState{Active,Disabled}` 동일 구조 + `SetLaserCorridorState` | 수정 |
| `WxGimmick.{h,cpp}` | `bTriggered`·`MarkTriggered`·`OnRep_bTriggered`·`GetLifetimeReplicatedProps`·`Net/UnrealNetwork.h` 삭제, doc 갱신 | 수정 |
| `WxDoor.h` | "base bTriggered" 문구 제거 | 수정 |
| 메모리 `gimmick-state-enum-pattern.md` | bTriggered 완전 제거·소비자 이전·StateTree 유무 두 갈래 갱신 | 수정 |

### 구현·결정과 그 이유
- **Elevator 패턴 일괄 적용**: 4개 소비자는 StateTree 가 없고 C++ `ApplyState` 가 상태를 소비하므로, Door(폴링)가 아니라 Elevator 처럼 `ReplicatedUsing=OnRep_State`(클라 OnRep→ApplyState) + 권위 setter(`if(!HasAuthority()||State==NewState) return; State=NewState; ApplyState();`)로 통일했다. StateTree Enum Compare 바인딩이 없어 State 를 `VisibleAnywhere`/`AllowPrivateAccess` 로 노출할 필요가 없어 private 으로 두었다(Door/Elevator 와 다른 점).
- **동작 1:1 보존**: 기존 `if (bTriggered)` → `if (State == 발동값)`, `MarkTriggered()` → `SetXxxState(발동값)` 로 그대로 치환. TreasureChest 의 라이브 발동(멀티캐스트 애니 재생) 순서와 `!IsPlaying()` 스냅 분기, AlarmConsole 의 FX-전피어/State-권위 분리, LaserCorridor 의 Active 일 때 타이머 가동 분기까지 의미 동일.
- **base 슬림화**: bTriggered 제거로 base 에 복제 prop 이 사라져 `GetLifetimeReplicatedProps` 오버라이드와 `Net/UnrealNetwork.h` include 를 통째로 제거. 자식의 `Super::GetLifetimeReplicatedProps` 는 `AActor` 버전으로 정상 귀결(빌드로 확인).
- **검증**: WxEditor(Development) 빌드 `Result: Succeeded`. WxWorld·WxGame 양 모듈 컴파일·링크 완료, 경고는 변경과 무관한 엔진 deprecation(C4996)뿐. (빌드 전 한글 worklog 경로의 UBT 비ASCII 크래시 예방 차 `core.quotepath false` 설정.)

### 계획 대비 달라진 점
- 계획대로. (메모리 갱신 시 "StateTree 유무 두 갈래" 구분을 한 항목으로 명시한 정도가 추가.)

### 후속 과제
- 없음. (SaveGame 키가 `bTriggered`→`State` 로 바뀌어 기존 개발 세이브의 발동 상태는 마이그레이션되지 않으나, 개발 데이터라 무시 가능.)
