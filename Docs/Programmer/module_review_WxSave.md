# WxSave — 코드 리뷰

> 13개 파일 규모의 작고 잘 정돈된 모듈이다. 코딩·모듈 규칙 준수도가 높고(WxCore 외 Wx 의존 없음, 전 파일 Copyright, `Super::` 호출·`Handle` prefix 대부분 준수), 직렬화 버전 헤더·트래블 가드처럼 까다로운 자리엔 근거 주석이 붙어 있다. 남은 문제는 크래시가 아니라 "실패·중첩 경로에서 조용히 세이브가 비거나 완료 신호가 어긋나는" 쪽에 몰려 있다. 이번 리뷰는 13개 소스 전부를 읽고 직렬화(`CaptureActor`/`RestoreActor`)·저장 완료 신호·플레이어 스냅샷 플러시·스폰 컴포넌트 등록 타이밍을 UE 5.8 엔진 소스와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 7 |

## 결과

### 1. 🟡 `FlushPlayerStats` 가 캡처에 실패해도 기존 스탯을 먼저 지운다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:194-196`
- **범주**: 버그/정확성
- **문제**: `PlayerStats.Reset()` 을 먼저 하고 `CapturePlayerStats` 가 아무것도 담지 못하면 `bHasPlayerStats` 가 `false` 로 떨어져 **직전까지 저장돼 있던 스탯이 통째로 사라진다**. `CapturePlayerStats` 는 ASC 를 못 찾으면 조용히 return 하는데(`:203-207`), `GetAbilitySystemComponentFromActor` 는 `IAbilitySystemInterface` 를 구현하지 않은 폰에서 null 을 답한다. 즉 플레이어가 ASC 없는 폰(탈것·터렛·연출용 폰 등)에 빙의한 상태에서 체크포인트 오토세이브가 걸리면 그 순간 세이브의 스탯이 비워지고, 이후 로드는 데이터테이블 기본 스탯으로 돌아간다 — 경고 로그도 없이 "어트리뷰트 0개 캡처"만 남는다. 같은 함수의 폰 부재 경로(`:187-192`)와 `FlushPlayerTransform`(`:161-169`)은 "이전 캡처를 보존한다"는 반대 규약을 지키고 있어 비대칭이다.
- **제안**: 로컬 `TMap` 에 캡처한 뒤 `Num() > 0` 일 때만 `SaveGame->PlayerStats` 로 커밋한다(비면 이전 값·`bHasPlayerStats` 유지 + Warning 로그).
- **확신도**: 중간(현재 `AWxCharacterBase` 는 ASC 를 기본 서브오브젝트로 들고 있어 통상 경로에선 재현되지 않는다 — ASC 없는 폰이 등장하는 순간 터진다)

### 2. 🟡 `bSaveInProgress`/`OnSaveCompleted` 가 "저장은 한 번에 하나" 전제라 중첩 시 신호가 어긋난다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:150`, `:278-281`, `:312-319`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:48-58`
- **범주**: 버그/정확성
- **문제**: 진행 상태가 bool 하나이고 완료 통지는 "발화하면서 스스로 비우는" 멀티캐스트 하나다. 요청을 세지 않으므로 저장 A 의 기록이 끝나기 전에 저장 B 가 걸리면(체크포인트 오토세이브 진행 중 메뉴 저장, 체크포인트 연속 통과 등) A 의 완료 콜백이 `bSaveInProgress` 를 내리고 `Broadcast()` 해 B 의 대기자까지 깨운다. `FWxStateTreeTask_SaveGame` 은 `bShouldCallTick = false` 로 이 신호에만 의존하므로, B 를 요청한 상태가 실제 디스크 기록 전에 `Succeeded` 로 빠진다(데이터 손실은 없다 — `AsyncSaveGameToSlot` 은 호출 시점에 동기 직렬화를 마치므로 A·B 모두 각자의 스냅샷을 쓴다). 부수적으로 `Broadcast()` 뒤에 `Clear()` 를 부르므로, 브로드캐스트 도중 새로 등록한 청자는 곧바로 지워져 영영 신호를 못 받는다 — 현재 유일한 구독자인 ST 태스크는 `FinishTask` 가 전이를 다음 틱으로 미루므로(엔진 `StateTreeAsyncExecutionContext.cpp:243-247`) 도달하지 않지만, `OnSaveCompleted` 가 public 인 이상 남는 함정이다.
- **제안**: 진행 카운터(또는 요청별 토큰)로 바꿔 콜백이 자기 요청만 차감하게 한다. 최소한 `FinishSaveInProgress` 에서 `MoveTemp` 으로 델리게이트를 로컬로 옮긴 뒤 브로드캐스트해 중첩 등록이 지워지지 않게 한다.
- **확신도**: 높음(메커니즘 확실, 영향 범위는 중첩 빈도에 달림)

### 3. 🟡 월드 서브시스템이 없으면 플러시 없이 슬롯을 덮어쓴다 — 클라이언트에서 세이브 파일이 비워질 수 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:161-170`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:42-52`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp:68-74`
- **범주**: 설계/구조
- **문제**: `UWxSaveWorldSubsystem::ShouldCreateSubsystem` 이 `NM_Client` 월드를 제외하므로, 클라이언트에서 `SaveToFile` 이 불리면 `else` 분기로 빠져 **라이브 상태 플러시 없이** 인메모리 SaveGame(클라에선 `Initialize` 가 만든 빈 부트스트랩 슬롯)을 그대로 디스크에 쓴다. `UWxSaveLibrary::SaveToFile` 은 BP 진입점인데 권위 게이트가 없어(ST 태스크만 `HasAuthority` 를 본다) UI 저장 버튼이 클라에서 눌리면 로컬 슬롯 파일이 빈 세이브로 덮인다. 헤더는 이 분기를 "트랜지션 등"으로 설명하지만 실제로 도달 가능한 주 경로는 클라이언트다. 경고 로그조차 없어 사후 진단도 어렵다.
- **제안**: `else` 분기에 Warning 로그를 남기고, 스탠드얼론 전제를 유지할 거라면 `SaveToFile` 진입에서 권위/월드 서브시스템 부재를 판정해 중단한다(`UWxSaveLibrary` 도 같은 게이트를 타게 된다).
- **확신도**: 중간(스탠드얼론 싱글 전제라면 도달하지 않는 의도된 단순화일 수 있음)

### 4. 🟡 월드 서브시스템 전역에 SaveGame 조회 전문(前文)이 8회 복제돼 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:106-113`, `:123-131`, `:145-152`, `:177-184`, `:425-431`, `:457-463`, `:481-487`, `:511-516`
- **범주**: 중복/복잡도
- **문제**: `GetWorld() → GetGameInstance() → GetSubsystem<UWxSaveGameSubsystem>() → GetSaveGame()` 사슬과 null 가드가 8개 함수에 그대로 반복된다. 사본마다 실패 처리도 갈린다 — `FlushMapTravelData`/`FlushSavableActors` 는 경고를 남기고 나머지는 조용히 return 한다. 새 핸들러를 추가할 때 어느 사본을 복사하느냐로 진단 가능성이 달라지고, 조회 규칙이 바뀌면 8곳을 고쳐야 한다.
- **제안**: private `UWxSaveGameSubsystem* GetGameSubsystem() const` / `UWxSaveGame* GetActiveSaveGame() const` 두 헬퍼로 접고 로그 정책도 그 안에서 통일한다.
- **확신도**: 높음

### 5. 🟢 델리게이트 콜백 `ContinueSaveToFileToDisk` 의 `Handle` prefix 누락과 불필요한 람다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:163-165`, `:292-309`
- **범주**: 규칙 위반
- **문제**: (a) `ContinueSaveToFileToDisk` 는 `FOnSaveFlushComplete::FDelegate::CreateUObject` 로 바인딩되는 콜백인데 `Handle` prefix 가 없다(코딩 규칙 4). (b) `AsyncSaveGameToSlot` 완료 콜백이 `CreateLambda` + `TWeakObjectPtr` 수동 캡처로 작성돼 있는데, `FAsyncSaveGameToSlotDelegate` 는 일반 델리게이트라 `CreateUObject` 로 멤버 함수를 물리면 약참조 수명 처리를 엔진이 대신한다 — 람다가 필요한 자리가 아니다(코딩 규칙 3). 같은 파일 `:12-26` 의 콘솔 명령 람다와 `WxStateTreeTask_SaveGame.cpp:55` 의 약 실행 컨텍스트 람다는 대안이 없어 해당하지 않는다.
- **제안**: `ContinueSaveToFileToDisk` → `HandleSaveFlushComplete` 로 개명하고, 비동기 기록 콜백은 `HandleAsyncSaveComplete(const FString&, int32, bool)` 멤버로 빼 `CreateUObject(this, ...)` 로 바인딩한다.
- **확신도**: 높음

### 6. 🟢 월드 초기화 복원이 액터당 `FindSavable` 을 두 번 돈다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:436-444` (재호출은 `RestoreActor` 내부 `:342`)
- **범주**: 성능/안전, 중복/복잡도
- **문제**: 로그용 `SavableCount` 를 세려고 `if (FindSavable(Actor))` 를 돌린 뒤, 그 안에서 `RestoreActor` 가 같은 `FindSavable` 을 다시 호출한다. `FindSavable` 은 `Cast` 실패 시 `FindComponentByInterface` 로 액터의 전 컴포넌트를 훑으므로(`:101`) 공짜가 아니고, 이 루프는 월드의 모든 액터가 대상이다. 같은 성격의 전 액터 순회가 저장할 때마다(`FlushSavableActors:135-138`), 맵을 뜰 때마다(`HandleWorldBeginTearDown:528`) 반복된다 — 오픈월드 규모가 커지면 체크포인트 저장 히치로 드러날 자리다.
- **제안**: 최소한 이중 호출은 없앤다(`RestoreActor` 가 "savable 이었는가"를 out 파라미터로 알려주게). 액터 수가 커지면 `IWxSavable` 액터를 BeginPlay/EndPlay 에 등록하는 레지스트리로 전 액터 순회 자체를 걷어내는 것을 검토한다.
- **확신도**: 높음(중복), 중간(성능 — 실제 액터 수에 달림)

### 7. 🟢 `UWxPlayerSpawnComponent` 의 캐치업 경로가 멱등하지 않고 빙의 구독이 해제되지 않는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:32-36`, `:39-45`, `:56`, `:80-90`, `:93-101`
- **범주**: 버그/정확성
- **문제**: `OnRegister` 는 `PostLoginHandle.IsValid()` 로 중복 구독만 막는데 `OnUnregister` 가 그 핸들을 `Reset()` 하므로 재등록 시 가드가 풀린다. 재등록 시점에 오너 PC 에 `PlayerState` 가 있으면 캐치업이 `HandleGameModePostLogin` 을 다시 실행해 — (a) `APlayerStart` 마커를 하나 더 스폰하고 `StartSpot` 을 교체, (b) `SetInitialLocationAndRotation` 으로 플레이 중인 PC 를 저장 좌표로 되돌리며, (c) `OnPossessedPawnChanged` 에 중복 바인딩을 건다(`AddDynamic` 은 중복 제거를 하지 않는다). 또 이 구독은 `OnUnregister` 에서 해제되지 않고 **모든** 빙의 변경에 반응하므로, 게임 중 재빙의 경로가 생기면(탈것 하차 등) 그때마다 마지막 저장 시점 스탯이 현재 값 위에 덮인다. 초기 주입 경로 자체는 안전함을 확인했다 — `AWxPlayerController::PreInitializeComponents`(`Source/WxGame/Controller/WxPlayerController.cpp:14-19`)에서 receiver 를 등록하므로 동기 부착 시점엔 `PlayerState` 가 없어 캐치업과 실제 PostLogin 이 둘 다 도는 일은 없다.
- **제안**: "이미 셋업했다" 플래그(또는 스폰한 마커의 약참조)로 캐치업을 1회로 묶고, `OnUnregister` 에서 `OnPossessedPawnChanged` 를 `RemoveDynamic` 해 등록/해제를 대칭으로 만든다. 스탯 복원이 "로드/부활 직후 1회"가 의도라면 첫 적용 후 구독을 끊는다.
- **확신도**: 중간(경로는 확실, 현재 ControllerComponent 재등록·플레이어 재빙의 빈도는 낮음)

### 8. 🟢 역직렬화 실패를 감지하지 않고 복원 성공으로 보고한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:375-410`, `:412-415`
- **범주**: 버그/정확성
- **문제**: `FMemoryReader` 가 스트림 끝을 넘어 읽거나 타입이 맞지 않으면 `ArIsError` 가 서는데, 액터·컴포넌트 어느 리더도 `IsError()` 를 확인하지 않는다. 반쯤 덮인 상태 그대로 `OnSaveRestored()` 를 부르고 `true`(복원 성공)를 반환하므로 로그에도 정상으로 집계된다. 태그 기반 프로퍼티 직렬화라 `UPROPERTY(SaveGame)` 필드 추가·제거·순서 변경은 안전하지만(그래서 이 설계가 성립한다), 필드 **타입** 변경이나 레코드 손상은 이 경로로 조용히 흘러간다.
- **제안**: 각 `Serialize` 뒤에 `MemReader.IsError()` 를 확인해 Warning 로그를 남기고, 최소한 액터 본체 실패 시엔 `OnSaveRestored()` 호출을 건너뛴다.
- **확신도**: 중간(현실적 트리거가 드묾 — 슬롯 파일 전체 손상은 `LoadFromFile` 이 이미 빈 슬롯으로 폴백한다)

### 9. 🟢 `GetStableMapPackageName` 이 public static 인데 `World` null 검사가 없다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:250-254`
- **범주**: 성능/안전
- **문제**: `World->GetOutermost()->GetName()` 을 무검사로 역참조한다. 현재 호출부는 모두 상위에서 World 를 검증했거나(`TravelFromSaveFile:111`, `TryGetPlayerTransform:205`, `ReportTravelFromSaveFileComplete:236`) null 이면 도달하지 않는 구조라(`FlushMapTravelData` 는 World null 이면 GameSubsystem 도 null 이라 조기 return) 지금은 안전하다. 다만 헤더가 "세이브의 맵 키 표현을 한 곳에서 강제한다"며 공개 API 로 선언하고 있어, 모듈 밖/미래 호출부가 검증 없이 부르기 쉽다.
- **제안**: 선두에 `if (!World) { return NAME_None; }` 를 넣는다.
- **확신도**: 높음(현재 크래시 없음 — 회귀 방지 성격)

### 10. 🟢 `GameplayTags` 는 쓰이지 않는 의존, `GameplayAbilities` 는 private 로 충분하다
- **위치**: `Plugins/WxSave/Source/WxSave/WxSave.Build.cs:16-17`
- **범주**: 설계/구조
- **문제**: 모듈 전체에 `GameplayTag` 식별자가 한 번도 등장하지 않는다(소스 전량 검색 기준 0건). `GameplayAbilities` 도 `WxSaveWorldSubsystem.cpp` 에서만 쓰이고 public 헤더 어디에도 GAS 타입이 노출되지 않는다. 지금은 GAS 가 GameplayTags 를 끌고 오므로 빌드에 영향은 없다.
- **제안**: `GameplayTags` 제거, `GameplayAbilities` 는 `PrivateDependencyModuleNames` 로 이동. (`StateTreeModule` 은 public 헤더 `WxStateTreeTask_SaveGame.h` 가 `StateTreeTaskBase.h` 를 포함하므로 public 유지가 맞다.)
- **확신도**: 높음

### 11. 🟢 어트리뷰트 스냅샷 키가 프로퍼티 이름뿐이라 AttributeSet 간 충돌 여지가 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:217-227`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h:84`
- **범주**: 설계/구조
- **문제**: `CapturePlayerStats` 는 모든 `SpawnedAttributes` 를 순회하며 `It->GetFName()` 을 평면 `TMap<FName, float>` 키로 쓴다. 서로 다른 AttributeSet 이 같은 이름의 어트리뷰트를 가지면 나중 것이 앞의 것을 덮고, `ApplyPlayerStats` 는 그 하나의 값을 두 세트 모두에 적용한다. AttributeSet 이 `UWxCombatAttributeSet` 하나뿐인 현재는 발생하지 않는다.
- **제안**: 세트가 늘어날 계획이면 키를 `"<AttributeSetClass>.<PropertyName>"` 로 승격한다. 당장은 이 전제(단일 세트)를 `PlayerStats` 주석에 명시해 둔다.
- **확신도**: 낮음(의도된 단순화일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/WxSave.uplugin`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`
- **엔진 소스와 대조해 기각한 항목**(재조사 낭비를 막기 위해 남긴다):
  - `CaptureActor`/`RestoreActor` 의 커스텀 버전 헤더는 정상이다. `FArchiveProxy` 가 `GetCustomVersions`/`SetCustomVersions` 를 내부 archive 로 포워딩하고(`ArchiveProxy.h:204-217`) `FArchive::UsingCustomVersion` 이 그 가상 함수를 경유하므로(`Archive.cpp:628-638`) 프록시를 씌워도 버전이 `FMemoryWriter` 에 정상 누적된다. 리더 쪽도 프록시 생성 **전에** `SetUEVer`/`SetCustomVersions` 를 호출하는 순서가 맞다.
  - `UGameplayStatics::AsyncSaveGameToSlot` 은 호출 스레드에서 `SaveGameToMemory` 로 동기 직렬화한 뒤 바이트만 비동기로 쓰고, 실패 시에도 델리게이트를 반드시 실행한다(`GameplayStatics.cpp:2403-2426`). 따라서 기록 대기 중 SaveGame 을 수정해도 데이터 레이스가 없고, `bSaveInProgress` 가 영구히 걸려 ST 태스크가 갇히는 경로도 없다.
  - `UWxPlayerSpawnComponent` 의 `StartSpot` 주입 타이밍은 성립한다. `FGameModeEvents::GameModePostLoginEvent` 는 `AGameModeBase::OnPostLogin` 에서, 즉 폰을 스폰하는 `HandleStartingNewPlayer` **이전**에 브로드캐스트된다(`GameModeBase.cpp:1032-1035`, `1042-1049`). 캐치업 경로도 `AWxGameMode::HandleStartingNewPlayer_Implementation` 이 Experience 로드까지 스폰을 미루므로(`Source/WxGame/Framework/WxGameMode.cpp:55-64`) 늦지 않는다.
  - `FSoftObjectPath` 에 패키지 경로만 담는 `FlushMapTravelData` 의 왕복(`IsNull()`·`GetAssetPath().GetPackageName()`)은 성립한다.
  - `WxStateTreeTask_SaveGame.h:43` 의 헤더 인라인 정의는 코딩 규칙 6 위반처럼 보이나, 같은 파일 13행에 예외 근거(반환 타입 표기뿐이고 엔진 StateTree 도 동일 형태)가 명시돼 있어 재론하지 않았다.
- **미검토 / 한계**: `IWxSavable` 구현체(WxWorld 의 `AWxSpawner`·`UWxGimmickStateTreeComponent`)는 복원 멱등성 판단에 필요한 만큼만 보았고 그 자체는 리뷰하지 않았다. 한 액터가 `OnWorldInitializedActors` 와 `LevelAddedToWorld` 양쪽에 걸려 `OnSaveRestored()` 가 두 번 불릴 수 있는지는 World Partition 초기 셀 로딩 순서에 달려 있어 코드만으로 확정하지 못했다(두 구현체 모두 재호출에 방어적이라 발견으로 올리지 않았다). 실제 세이브 파일 왕복·이기종 빌드 간 레코드 호환은 빌드/에디터 실행 없이 검증할 수 없어 정적 분석에 그쳤고, `UWxSaveLibrary` 를 호출하는 BP/WBP 측 사용 패턴도 범위 밖이다.

---
*문서 기준 커밋 `e9440f73` · 리뷰일 2026-08-15 · 소스 13파일 — `/module-review`로 갱신*
