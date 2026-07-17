# 부활 지점을 StartSpot 주입으로 — AWxGameMode override 를 InitGame 하나로

## 계획

### 목표

`AWxGameMode` 에 `InitGame` 을 제외한 override 를 남기지 않는다. 현재 남은 둘은 `RestartPlayer`(저장 부활 지점 재시작)와 `FinishRestartPlayer`(저장 스탯 복원)다.

엔진에 이미 이 용도의 메커니즘이 있다. `AController::StartSpot`("Actor marking where this controller spawned in")이 세팅돼 있으면 `FindPlayerStart` 가 `ShouldSpawnAtStartSpot` 을 통해 `ChoosePlayerStart` 를 아예 타지 않고 그 액터를 반환한다. `FGameModeEvents::GameModePostLoginEvent` 는 `HandleStartingNewPlayer`(→`RestartPlayer`)보다 먼저 브로드캐스트되므로, 여기서 StartSpot 을 꽂으면 스폰·컨트롤 로테이션을 엔진이 전부 처리한다. 엔진 자신도 PIE "Play From Here" 를 같은 방식(좌표에 PlayerStart 런타임 스폰)으로 구현한다.

월드파티션에서도 같은 방향이다. PC 는 그 자체로 스트리밍 소스이고 빙의 전 ViewTarget 은 자기 자신이라, `UpdatePlayerStartSpot` 이 PC 를 StartSpot 자리로 옮기면 폰보다 셀이 먼저 로딩된다. 현재의 `RestartPlayerAtTransform` 우회는 PC 를 레벨 PlayerStart 에 남겨둔 채 폰만 저장 좌표에 스폰해 그 지연을 만든다.

낙하 안전망(그리드 `bBlockOnSlowStreaming` 등)은 이번 범위 밖.

### 수정 범위

WxSave 는 변경 없다 — 기존 `TryGetRespawnTransform` / `ApplySavedPlayerStats` 를 그대로 쓴다.

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxPlayerSpawnSubsystem.h/.cpp` | `UWxPlayerSpawnSubsystem : UWorldSubsystem`. `FGameModeEvents::OnGameModePostLoginEvent()` 구독. 핸들러: 자기 월드 필터 → `APlayerStartPIE` 있으면 skip → `TryGetRespawnTransform` 실패면 skip → Yaw 만 남긴 회전으로 `APlayerStart` 를 `RF_Transient` 스폰 → `PC->StartSpot = Marker` + `PC->SetInitialLocationAndRotation(...)` | 신규 |
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | `RestartPlayer`·`FinishRestartPlayer` override 와 `TryGetSavedRespawnTransform` 제거, WxSave include 제거. `InitGame` 과 `FrameworkComponents`·`ComponentRequestHandles` 만 남김 | 수정 |
| `Source/WxGame/Controller/WxPlayerController.h/.cpp` | 기존 `OnPossess` 의 `Super::` 직후, `IsLocalController` 게이트 앞에 `ApplySavedPlayerStats(InPawn)` 추가 | 수정 |

### 접근 방식

- **저장 지점이 있으면 로그인 시 그 좌표에 `APlayerStart` 를 런타임 스폰하고 StartSpot 에 꽂는다**: 폰 스폰 위치·컨트롤 로테이션·사망 후 부활을 전부 엔진 기본 경로가 처리한다.

- **마커 갱신 문제는 없다**: 사망 부활이 맵 리로드에 의존하므로 부활마다 새 월드 → 새 PostLogin → 세이브에서 마커 재생성. 세이브가 단일 원천이고 마커는 월드 수명의 파생물이다.

- **마커는 `APlayerStart` 를 그대로 쓴다**: 전용 클래스가 불필요하고 `bIsSpatiallyLoaded = false` 를 생성자에서 물려받아 WP 에서 항상 로드된다. `APlayerStartPIE` 는 쓰면 안 된다 — PIE "여기서 플레이" 감지와 충돌한다(PersistenceLab 샘플이 그 방식을 쓰지만 샘플 자신이 "not guaranteed by any hard rule" 이라 경고한다).

- **PIE 우선 규칙은 주입을 건너뛰는 것으로 표현한다**: `APlayerStartPIE` 가 있으면 StartSpot 을 꽂지 않고 엔진 `ChoosePlayerStart` 가 그걸 최우선으로 고르게 둔다.

- **스탯 복원 보장은 유지된다**: `AController::Possess` 가 authority 전용이라 서버 보장이 그대로고, `Super::OnPossess` 가 `PossessedBy`(→ `InitAbilitySystem`)를 끝낸 뒤 돌아온다. 타이밍이 `SetPlayerDefaults` 앞으로 당겨지지만 그 함수는 오버라이드가 없어 무관하다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxPlayerSpawnSubsystem.h/.cpp` | PostLogin 에서 저장 좌표에 `APlayerStart` 마커를 스폰해 `StartSpot` 주입 + PC 선이동 | 신규 |
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | `RestartPlayer`·`FinishRestartPlayer` override 와 `TryGetSavedRespawnTransform` 제거, WxSave include 제거. `InitGame` 만 남음 | 수정 |
| `Source/WxGame/Controller/WxPlayerController.h/.cpp` | `OnPossess` 의 `Super::` 직후·로컬 게이트 앞에 `ApplySavedPlayerStats`. 헤더 주석에 빙의 시점 책임 명시 | 수정 |

### 구현·결정과 그 이유

- **엔진이 이미 가진 메커니즘을 찾아 쓴 것이지 우회로를 만든 게 아니다**: `AController::StartSpot` 은 주석부터 "Actor marking where this controller spawned in" 이다. 이게 세팅되면 `FindPlayerStart` 가 `ShouldSpawnAtStartSpot` 을 통해 `ChoosePlayerStart` 를 건너뛰고 그 액터를 쓴다. 엔진의 PIE "Play From Here" 도 좌표에 PlayerStart 를 런타임 스폰하는 같은 해법이다.

- **override 제거와 월드파티션 안전이 같은 방향이었다**: PC 는 그 자체로 스트리밍 소스이고 빙의 전 ViewTarget 이 자기 자신이다. 직전의 `RestartPlayerAtTransform` 우회는 PC 를 레벨 PlayerStart 에 남긴 채 폰만 저장 좌표에 스폰해, 그 셀 로딩이 폰 생성 후에야 시작됐다. StartSpot 경로는 `UpdatePlayerStartSpot` 이 PC 를 먼저 옮겨 이 지연을 없앤다.

- **`APlayerStartPIE` 를 쓰지 않았다**: PersistenceLab 샘플은 그 방식이지만, 샘플 자신이 "not guaranteed by any hard rule" 이라 경고하며 GameMode override 를 권한다. 게다가 우리는 PIE "여기서 플레이" 를 `TActorIterator<APlayerStartPIE>` 로 감지하므로, 그걸 직접 스폰하면 저장 지점이 영구히 무시된다. 대신 평범한 `APlayerStart` 를 쓰고 PIE 일 땐 주입을 건너뛴다.

- **마커 갱신 로직이 필요 없다**: 사망 부활이 맵 리로드에 의존해 부활마다 새 PostLogin 을 타므로, 세이브가 단일 원천이고 마커는 월드 수명의 파생물이다. 체크포인트 접촉 시 마커를 동기화할 일이 없다.

- **스탯 복원은 PC 의 빙의 시점 책임에 합류**: `OnPossess` 는 이미 HUD 푸시·사망 구독을 모아둔 곳이다. `Super::OnPossess` 가 `PossessedBy`(→`InitAbilitySystem`)를 끝낸 뒤라 ASC 보장이 동일하고, `Possess` 가 authority 전용이라 서버 보장도 유지된다. 로컬 컨트롤러 게이트 앞에 둔 건 authority 작업이지 로컬 표시 작업이 아니기 때문이다.

### 계획 대비 달라진 점
- 계획대로. 마커 스폰에 `AlwaysSpawn` 충돌 처리를 지정한 것은 계획에 없었으나, 저장 지점이 지오메트리에 걸쳐 있어도 마커 생성이 실패하면 안 되기 때문이다(마커는 위치 표식일 뿐 물리 충돌 대상이 아니다).

### 후속 과제
- **런타임 동작 미검증**: 스폰 경로가 통째로 바뀌었으므로 에디터 확인이 남았다 — 체크포인트 저장 후 사망/로드 시 그 지점·그 방향으로 스탯과 함께 부활하는지, 신규 세션이 레벨 PlayerStart 로 스폰되는지, PIE "여기서 플레이" 가 저장 위치를 무시하는지. WP 레벨(`LV_ChangYoung`·`LV_Dungeon1`)에서도 확인하면 PC 선이동 효과를 볼 수 있다.
- **WP 낙하 안전망 미적용**(사용자 결정으로 범위 밖): 그리드의 `bBlockOnSlowStreaming` 이 기본 false 이고, `AGameModeBase` 상속이라 `AGameMode::HandleMatchHasStarted` 의 `BlockTillLevelStreamingCompleted` 도 없다. StartSpot 주입으로 PC 선이동 이득은 얻었지만 셀 로딩 완료를 보장하진 않는다.
- **GM 에셋의 죽은 `WxPlayerSpawningComponent` 참조**: `GM_Combat`·`GM_ChangYoung` 의 `FrameworkComponents` 배열에 남은 빈 엔트리 — 사용자가 직접 정리 예정.
