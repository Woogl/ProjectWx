# WxSave — 코드 리뷰

> 13개 파일 규모의 작고 잘 정돈된 모듈이다. 코딩·모듈 규칙 준수도가 높고(WxCore 외 의존 없음, 전 파일 Copyright, Super 호출·Handle prefix 대부분 준수), 직렬화 버전 헤더·트래블 가드 같은 까다로운 부분에 근거 주석이 붙어 있다. 이번 리뷰는 13개 소스 전부를 읽고, 직렬화(`CaptureActor`/`RestoreActor`)·트래블 가드·GAS 스탯 복원·스폰 컴포넌트의 등록 타이밍을 엔진 소스와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 `bSaveInProgress` 단일 플래그가 겹치는 저장 요청에서 잘못된 완료 신호를 낸다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:150`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:292-309`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:61`
- **범주**: 버그/정확성
- **문제**: `SaveToFile` 은 진입할 때마다 `bSaveInProgress = true` 로 덮어쓰고, 비동기 기록 콜백은 어느 요청의 콜백이든 무조건 `false` 로 내린다. 요청을 세지 않으므로 저장 A 의 기록이 끝나기 전에 저장 B 가 걸리면, A 의 콜백이 B 의 완료 신호까지 내려버린다. `FWxStateTreeTask_SaveGame::Tick` 은 이 플래그 하나만 폴링하므로(같은 서브시스템 인스턴스라 태스크마다 구분이 없다) B 의 상태가 실제 디스크 기록 전에 `Succeeded` 로 빠진다. 체크포인트를 연속으로 밟거나 체크포인트 저장 중 메뉴 저장이 겹치면 재현된다. 데이터 손실은 없으나("저장 완료" 이후 연출/전이가 근거 없이 진행됨) 이 태스크가 존재하는 이유 자체를 무너뜨린다.
- **제안**: bool 대신 진행 중 요청 카운터(또는 요청별 토큰)로 바꿔 콜백이 자기 요청만 차감하게 한다. 겹침을 허용하지 않을 생각이면 `SaveToFile` 진입 시 `bSaveInProgress` 면 큐잉/거부하도록 명시한다.
- **확신도**: 높음(메커니즘은 확실, 실제 영향 범위는 겹침 빈도에 달림)

### 2. 🟡 월드 서브시스템 전역에 SaveGame 조회 전문(前文)이 8회 복제돼 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:106-113`, `:123-131`, `:145-152`, `:177-184`, `:425-431`, `:457-463`, `:481-487`, `:511-516`
- **범주**: 중복/복잡도
- **문제**: `GetWorld() → GetGameInstance() → GetSubsystem<UWxSaveGameSubsystem>() → GetSaveGame()` 사슬과 null 가드가 8개 함수에 그대로 반복된다. 각 사본의 null 처리도 미묘하게 갈린다 — `FlushMapTravelData`/`FlushSavableActors` 는 경고 로그를 남기고, `FlushPlayerTransform`/`FlushPlayerStats`/핸들러들은 조용히 return 한다. 새 핸들러를 추가할 때 어느 사본을 복사하느냐로 진단 가능성이 달라지고, 조회 규칙이 바뀌면 8곳을 고쳐야 한다.
- **제안**: private `UWxSaveGameSubsystem* GetGameSubsystem() const` / `UWxSaveGame* GetActiveSaveGame() const` 두 헬퍼로 접고, 로그 정책도 그 안에서 하나로 통일한다.
- **확신도**: 높음

### 3. 🟢 델리게이트 콜백 `ContinueSaveToFileToDisk` 의 `Handle` prefix 누락과 불필요한 람다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:163-165`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:292-309`
- **범주**: 규칙 위반
- **문제**: 두 가지다. (a) `ContinueSaveToFileToDisk` 는 `FOnSaveFlushComplete::FDelegate::CreateUObject` 로 델리게이트에 바인딩되는 콜백인데 `Handle` prefix 가 없다(코딩 규칙 4). (b) `AsyncSaveGameToSlot` 의 완료 콜백이 `CreateLambda` + `TWeakObjectPtr` 캡처로 작성돼 있는데, `CreateUObject` 로 멤버 함수를 물리면 약참조 수명 처리는 엔진이 해주므로 람다가 필요 없다(코딩 규칙 3). 같은 파일 12-26행의 콘솔 명령 람다는 자유 함수를 만들지 않으려면 불가피하지만, 이쪽은 대안이 있다.
- **제안**: `ContinueSaveToFileToDisk` → `HandleSaveFlushComplete`(플러시 완료 콜백)로 개명하고, 비동기 기록 콜백은 `HandleAsyncSaveComplete(const FString&, int32, bool)` 멤버로 빼 `FAsyncSaveGameToSlotDelegate::CreateUObject(this, ...)` 로 바인딩한다.
- **확신도**: 높음

### 4. 🟢 `GetStableMapPackageName` 이 public static 인데 `World` null 검사가 없다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:250-254`
- **범주**: 성능/안전
- **문제**: `World->GetOutermost()->GetName()` 을 무검사로 역참조한다. 현재 호출부 4곳은 모두 상위에서 World 를 검증했거나(`TryGetPlayerTransform:205`, `ReportTravelFromSaveFileComplete:236`, `TravelFromSaveFile:111`) null 이면 도달하지 않는 구조라(`FlushMapTravelData` 는 World null 시 GameSubsystem 도 null 이라 조기 return) 지금은 안전하다. 다만 이 함수는 헤더에 "세이브의 맵 키 표현을 한 곳에서 강제한다"고 공개 API 로 선언돼 있어, 이 모듈 밖/미래 호출부가 검증 없이 부르기 쉽다. 파일의 다른 모든 함수가 방어적인 것과도 어긋난다.
- **제안**: 선두에 `if (!World) return NAME_None;` 를 넣는다.
- **확신도**: 높음(현재 크래시는 없음 — 회귀 방지 성격)

### 5. 🟢 월드 초기화 복원이 액터당 `FindSavable` 을 두 번 돈다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:436-444` (`RestoreActor` 내부 재호출은 `:342`)
- **범주**: 중복/복잡도, 성능/안전
- **문제**: 로그용 `SavableCount` 를 세려고 `if (FindSavable(Actor))` 를 돌린 뒤, 그 안에서 `RestoreActor` 가 같은 `FindSavable` 을 다시 호출한다. `FindSavable` 은 `Cast` 실패 시 `FindComponentByInterface` 로 액터의 전체 컴포넌트를 훑으므로(`:101`) 공짜가 아니고, 이 루프는 월드의 모든 액터를 대상으로 한다. 같은 성격의 전 액터 순회가 `FlushSavableActors`(`:135-138`)에서 저장할 때마다, `HandleWorldBeginTearDown`(`:528`)에서 맵을 뜰 때마다 반복된다 — 오픈월드 규모가 커질수록 체크포인트 저장 히치로 드러날 자리다.
- **제안**: 최소한 이중 호출은 없앤다(`RestoreActor` 가 "savable 이었는가"를 out 파라미터나 별도 반환값으로 알려주게). 액터 수가 커지면 `IWxSavable` 액터를 BeginPlay/EndPlay 에 등록하는 레지스트리로 전 액터 순회 자체를 걷어내는 것을 검토한다.
- **확신도**: 높음(중복), 중간(성능 — 실제 액터 수에 달림)

### 6. 🟢 `UWxPlayerSpawnComponent::OnRegister` 의 캐치업 경로가 멱등하지 않다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:32-36`, `:39-45`, `:56`, `:80-90`
- **범주**: 버그/정확성
- **문제**: `OnRegister` 는 `PostLoginHandle.IsValid()` 로 델리게이트 중복 구독만 막는데, `OnUnregister` 가 그 핸들을 `Reset()` 하므로 재등록 시 가드가 풀린다. 재등록 시점에 오너 PC 에 `PlayerState` 가 있으면 캐치업이 `HandleGameModePostLogin` 을 다시 실행해 — (a) `APlayerStart` 마커를 하나 더 스폰하고 `StartSpot` 을 교체, (b) `SetInitialLocationAndRotation` 으로 이미 플레이 중인 PC 를 저장 좌표로 되돌리며, (c) `OnPossessedPawnChanged` 에 중복 바인딩을 건다(`AddDynamic` 은 중복 제거를 하지 않는다). 초기 주입 경로는 안전함을 확인했다 — `AWxPlayerController::PreInitializeComponents`(`Source/WxGame/Controller/WxPlayerController.cpp:14-19`)에서 receiver 를 등록하므로 동기 부착 시점엔 `PlayerState` 가 아직 없고, 따라서 캐치업과 실제 PostLogin 이 둘 다 도는 일은 없다. 문제는 이후의 재등록뿐이라 발생 빈도는 낮다.
- **제안**: "이미 셋업했다"는 별도 플래그(또는 스폰한 마커의 약참조)를 두어 캐치업이 1회만 돌게 하고, `OnUnregister` 에서 `OnPossessedPawnChanged` 구독도 함께 해제해 등록/해제를 대칭으로 만든다.
- **확신도**: 중간(경로는 확실, ControllerComponent 가 실제로 재등록되는 빈도는 낮음)

### 7. 🟢 `GameplayTags` 는 선언만 되고 쓰이지 않는 의존이다
- **위치**: `Plugins/WxSave/Source/WxSave/WxSave.Build.cs:17`
- **범주**: 설계/구조
- **문제**: 모듈 전체에 `GameplayTag` 식별자가 한 번도 등장하지 않는다(소스 전량 검색 기준). 함께 선언된 `GameplayAbilities` 도 `WxSaveWorldSubsystem.cpp` 에서만 쓰이고 public 헤더 어디에도 GAS 타입이 노출되지 않으므로 `Private` 로 충분하다. 지금은 GAS 가 GameplayTags 를 끌고 오므로 빌드에 영향은 없다.
- **제안**: `GameplayTags` 제거, `GameplayAbilities` 는 `PrivateDependencyModuleNames` 로 이동. (`StateTreeModule` 은 public 헤더 `WxStateTreeTask_SaveGame.h` 가 `StateTreeTaskBase.h` 를 포함하므로 public 유지가 맞다.)
- **확신도**: 높음

### 8. 🟢 어트리뷰트 스냅샷 키가 프로퍼티 이름뿐이라 AttributeSet 간 충돌 여지가 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:217-227`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h:84`
- **범주**: 설계/구조
- **문제**: `CapturePlayerStats` 는 모든 `SpawnedAttributes` 를 순회하며 `It->GetFName()` 을 평면 `TMap<FName, float>` 키로 쓴다. 서로 다른 AttributeSet 이 같은 이름의 어트리뷰트를 가지면(향후 `Health` 를 가진 세트가 둘이 되는 식) 나중 것이 앞의 것을 덮고, `ApplyPlayerStats` 는 그 하나의 값을 두 세트 모두에 적용한다. AttributeSet 이 하나뿐인 현재는 발생하지 않는다.
- **제안**: 세트가 늘어날 계획이면 키를 `"<AttributeSetClass>.<PropertyName>"` 또는 `FGameplayAttribute` 식별자로 승격한다. 당장은 최소한 이 전제(단일 세트)를 `PlayerStats` 주석에 명시해 둔다.
- **확신도**: 낮음(현재 구성에서는 문제가 되지 않는 의도된 단순화일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`, `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`
- **엔진 소스와 대조해 문제 없음을 확인한 것들**(리뷰 중 의심했다가 기각한 항목 — 재조사 낭비를 막기 위해 남긴다):
  - `CaptureActor`/`RestoreActor` 의 커스텀 버전 헤더: `FArchiveProxy` 가 `GetCustomVersions`/`SetCustomVersions` 를 내부 archive 로 포워딩하고(`ArchiveProxy.h:204-217`) `FArchive::UsingCustomVersion` 이 그 가상 함수를 경유하므로, 프록시를 씌워도 버전이 `FMemoryWriter` 에 정상 누적된다. 리더 쪽도 프록시 생성 **전에** `SetUEVer`/`SetCustomVersions` 를 호출하는 순서가 맞다.
  - `FlushMapTravelData` 가 만드는 패키지 경로만의 `FSoftObjectPath`: `FTopLevelAssetPath::TrySetPath` 가 asset 이름 없는 패키지 참조를 지원하므로(`TopLevelAssetPath.cpp:146-153`) `IsNull()`·`GetAssetPath().GetPackageName()` 왕복이 성립한다.
  - `UWxSaveWorldSubsystem::ShouldCreateSubsystem` 의 `NM_Client` 제외: `InitializeSubsystems` 가 NetDriver 부착 전에 돌지만, `UWorld::AttemptDeriveFromURL` 이 아직 살아 있는 `PendingNetGame` URL 로 `NM_Client` 를 도출하므로(`World.cpp:9669-9679`) 클라 제외가 실제로 동작한다.
  - `AsyncSaveGameToSlot` 은 호출 스레드에서 `SaveGameToMemory` 로 동기 직렬화한 뒤 바이트만 비동기로 쓴다(`GameplayStatics.cpp:2403-2426`). 기록 대기 중 `SaveGame` 을 수정해도 데이터 레이스는 없다.
  - `FStateTreeTaskCommonBase` 의 `bShouldCallTick` 기본값이 true 이므로(`StateTreeTaskBase.h:23-37`) `FWxStateTreeTask_SaveGame` 의 빈 생성자로도 `Tick` 폴링이 성립한다(상태가 `Running` 에 갇히지 않는다).
  - `WxStateTreeTask_SaveGame.h:41` 의 헤더 인라인 정의는 코딩 규칙 6 위반처럼 보이나, 같은 파일 13행에 예외 근거가 명시돼 있어 재론하지 않았다.
- **미검토 / 한계**: `IWxSavable` 구현체(WxWorld 의 `AWxSpawner`·`UWxGimmickStateTreeComponent`)는 복원 멱등성 판단에 필요한 만큼만 보았고 그 자체를 리뷰하지 않았다. 특히 한 액터가 `OnWorldInitializedActors` 와 `LevelAddedToWorld` 양쪽에 걸려 `OnSaveRestored()` 가 두 번 불릴 수 있는지는 World Partition 초기 셀 로딩 순서에 달려 있어 코드만으로는 확정하지 못했다(두 구현체 모두 재호출에 방어적이라 발견으로 올리지 않았다). `UWxSaveLibrary` 를 호출하는 BP/WBP 측 사용 패턴도 범위 밖이다.

---
*문서 기준 커밋 `ebe6cffd` · 리뷰일 2026-08-12 · 소스 13파일 — `/module-review`로 갱신*
