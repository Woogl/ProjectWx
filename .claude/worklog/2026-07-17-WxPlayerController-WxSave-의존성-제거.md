# 재개 지점 주입을 ModularGameplay ControllerComponent 로 전환

## 계획

### 목표

직전 작업에서 재개 지점 주입을 `UWxPlayerSpawnSubsystem`(WorldSubsystem)으로 만들었고, 스탯 복원은 `AWxPlayerController::OnPossess` 에 남겨 PC 가 WxSave 를 직접 참조한다. 두 문제를 한 번에 정리한다.

프로젝트는 이미 ModularGameplay 를 채택했다 — `AWxGameMode::InitGame` 의 유일한 존재 이유가 `FrameworkComponents` 요청 등록이고, 거기 Controller 갈래 추론이 있지만 받을 액터가 `AWxGameState` 뿐이라 미사용이다. 재개 지점 주입을 그 패턴 밖의 WorldSubsystem 으로 두는 건 일관성이 없다. ControllerComponent 로 옮기면 컴포넌트가 오너 PC 를 알아 월드 필터가 자기-PC 필터로 바뀌고, `OnPossessedPawnChanged` 를 오너 PC 에 직접 구독해 PC 의 WxSave 참조가 사라진다.

### 수정 범위

WxSave·GameMode 는 변경 없다. 기존 `TryGetPlayerTransform` / `ApplySavedPlayerStats` 를 그대로 쓴다.

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxPlayerSpawnComponent.h/.cpp` | `UWxPlayerSpawnComponent : UControllerComponent`. `OnRegister` 에서 authority + 게임 월드일 때만 PostLogin 이벤트 구독. 핸들러: 자기 PC 필터 → `OnPossessedPawnChanged.AddDynamic`(조건 없이) → PIE skip → `TryGetPlayerTransform` skip → 마커 스폰 + `StartSpot` + `SetInitialLocationAndRotation`. `UFUNCTION()` 핸들러가 `ApplySavedPlayerStats(NewPawn)` | 신규 |
| `Source/WxGame/Framework/WxPlayerSpawnSubsystem.h/.cpp` | 삭제 — 컴포넌트로 이관 | 삭제 |
| `Source/WxGame/Controller/WxPlayerController.h/.cpp` | `PreInitializeComponents`/`EndPlay` 로 receiver 등록·해제(`AWxGameState` 와 동일 패턴). `OnPossess` 의 스탯 복원 블록과 WxSave·GameInstance include 제거 | 수정 |

### 접근 방식

- **훅은 `OnRegister` 에서 잡는다**: 이번의 핵심 제약이다. `SpawnPlayActor` → `Login` → `PostLogin` 이 `World->BeginPlay()` 보다 먼저라, 컴포넌트 `BeginPlay` 에서 구독하면 PostLogin 을 이미 놓친 뒤다. `AddGameFrameworkComponentReceiver`(PC 의 `PreInitializeComponents`) → `AddReceiverInternal` → `CreateComponentOnInstance` → `RegisterComponent()` 가 동기라 `OnRegister` 는 `InitNewPlayer`·`PostLogin` 보다 앞선다.

- **PostLogin 이벤트 구독은 그대로 필요**: `InitNewPlayer` 의 `UpdatePlayerStartSpot` 이 StartSpot 을 먼저 세팅하므로 그보다 이른 시점에 써봐야 덮어써진다. 컴포넌트로 옮겨도 훅은 동일하고 달라지는 건 사는 곳뿐이다.

- **구독은 조건 없이, StartSpot 주입만 조건부**: 저장 스탯은 맵·PIE 와 무관하고 재개 좌표는 맵 종속에 PIE 일 땐 건너뛴다. 구독을 조건부로 걸면 PIE 에서 스탯 복원이 조용히 사라지는 회귀가 된다.

- **authority 게이트는 오너로**: 클라 PC 에도 컴포넌트가 붙으므로 `GetOwner()->HasAuthority()` 로 막는다(서브시스템의 `!IsNetMode(NM_Client)` 대체). 클라 `OnRep_Pawn` 델리게이트도 이 게이트에 막힌다.

### 사용자 작업 (필수)

`GM_Combat`·`GM_ChangYoung` 의 `FrameworkComponents` 에 `WxPlayerSpawnComponent` 등록이 필요하다. C++ 로 불가하고 등록 없이는 재개 지점·스탯 복원이 조용히 동작하지 않는다. 앞으로 GameMode 추가 시마다 등록이 필요한 것이 서브시스템 대비 감수하는 비용이다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxPlayerSpawnComponent.h/.cpp` | `UWxPlayerSpawnComponent : UControllerComponent`. `OnRegister` 에서 PostLogin 구독, 재개 지점 마커 주입 + 빙의 구독으로 스탯 복원 | 신규 |
| `Source/WxGame/Framework/WxPlayerSpawnSubsystem.h/.cpp` | 삭제 — 컴포넌트로 이관 | 삭제 |
| `Source/WxGame/Controller/WxPlayerController.h/.cpp` | `PreInitializeComponents`/`EndPlay` 로 receiver 등록·해제. `OnPossess` 의 스탯 복원 블록·WxSave include 제거, 헤더 주석을 receiver 명시로 교체 | 수정 |

### 구현·결정과 그 이유

- **`BeginPlay` 가 아니라 `OnRegister` 에서 구독**: 이번의 핵심 함정이었다. `SpawnPlayActor`(→`Login`→`PostLogin`)가 `World->BeginPlay()` 보다 먼저라 컴포넌트 `BeginPlay` 시점엔 PostLogin 이 이미 지나갔다. 반면 PC 의 `PreInitializeComponents` → `AddGameFrameworkComponentReceiver` → `AddReceiverInternal` → `CreateComponentOnInstance` → `RegisterComponent()` 가 동기라 `OnRegister` 는 `InitNewPlayer`·`PostLogin` 보다 확실히 앞선다.

- **컴포넌트로 옮겨도 훅은 그대로 PostLogin**: `UpdatePlayerStartSpot` 이 StartSpot 을 먼저 확정하므로 더 이른 시점에 써봐야 덮어써진다. 달라진 건 사는 곳과 필터 정확도뿐이다 — 월드 비교 대신 오너 PC 비교가 되고, `OnPossessedPawnChanged` 를 오너 PC 에 직접 건다.

- **구독은 조건 없이, 마커 주입만 조건부**: 저장 스탯은 맵·PIE 와 무관하고 재개 좌표는 맵 종속에 PIE 일 땐 건너뛴다. 구독까지 조건부로 걸었다면 PIE "여기서 플레이" 에서 스탯 복원이 조용히 사라졌을 것이다.

- **authority 게이트를 오너로 이동**: 서브시스템의 `ShouldCreateSubsystem` + `!IsNetMode(NM_Client)` 를 `GetOwner()->HasAuthority()` 가 대체한다. 클라 PC 사본과 클라 `OnRep_Pawn` 델리게이트가 함께 막힌다.

- **미사용이던 Controller 갈래가 살아났다**: `InitGame` 의 부착 대상 추론은 GameState/Pawn/Controller/PlayerState 4갈래인데 receiver 가 `AWxGameState` 뿐이라 하나만 쓰이고 있었다. PC 를 receiver 로 등록하며 Controller 갈래가 실제 경로가 됐다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **에셋 등록 필요(미완)**: `GM_Combat`·`GM_ChangYoung` 의 `FrameworkComponents` 에 `WxPlayerSpawnComponent` 를 등록해야 동작한다. 등록 전까지는 재개 지점·스탯 복원이 에러 없이 조용히 동작하지 않는다. 죽은 `WxPlayerSpawningComponent` 빈 엔트리 정리도 같은 화면에서.
- **런타임 미검증**: 에셋 등록 후 확인 — 체크포인트 저장 → 사망/로드 시 그 지점·방향으로 스탯과 함께 재개, 신규 세션은 레벨 PlayerStart, PIE "여기서 플레이" 는 위치 무시하되 **스탯은 복원**(회귀 위험 지점), WP 레벨에서도 재개.
- **WP 낙하 안전망 미적용**: 그리드 `bBlockOnSlowStreaming` 기본 false, `AGameModeBase` 상속이라 `BlockTillLevelStreamingCompleted` 없음.
