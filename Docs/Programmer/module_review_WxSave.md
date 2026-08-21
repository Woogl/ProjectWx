# WxSave — 코드 리뷰

> 13개 파일의 작고 밀도 높은 모듈이다. 프로젝트 규칙 준수도가 높고(`WxCore` 외 Wx 의존 없음, 전 파일 Copyright 헤더, `Super::` 호출 전량 준수, `BlueprintCallable` 은 BP 라이브러리에만), 직렬화 버전 헤더·트래블 가드처럼 틀리기 쉬운 자리엔 근거 주석이 붙어 있다. 남은 문제는 크래시가 아니라 "실패·중첩 경로에서 세이브가 조용히 비거나 완료 신호가 어긋나는" 쪽에 몰려 있고, 그중 하나는 진행 상황을 영구히 잃을 수 있다. 이번 리뷰는 소스 13개 전부를 읽고 직렬화(`CaptureActor`/`RestoreActor`)·슬롯 I/O·저장 완료 신호·플레이어 스냅샷·스폰 주입 타이밍을 UE 5.8 엔진 소스와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 4 |
| 🟢 사소 | 7 |

## 결과

### 1. 🔴 판독 실패한 세이브 파일을 빈 슬롯으로 대체한 뒤 같은 슬롯에 되쓴다 — 진행 상황 영구 소실
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:79-92`, `:283-309`
- **범주**: 버그/정확성
- **문제**: `LoadFromFile` 은 `LoadGameFromSlot` 이 null 을 답하면 무조건 `StartNewSaveFile(TargetSlot, ...)` 로 **같은 슬롯 이름의 빈 SaveGame** 을 활성 슬롯에 앉힌다(`:90`). 그런데 `UGameplayStatics::LoadGameFromSlot` 은 파일 부재만이 아니라 헤더 손상, `UWxSaveGame` 서브클래스 소실·개명, 상위 엔진 버전으로 기록된 파일에서도 똑같이 null 을 답한다(`GameplayStatics.cpp` 의 `LoadGameFromSlot` → `LoadGameFromMemory` 는 모든 실패를 nullptr 하나로 접는다). 즉 "파일이 없다"와 "파일은 있는데 못 읽는다"가 구분되지 않는다. 후자일 때 남는 흔적은 `"파일 없음/손상"` 로그 한 줄(`:91`)뿐이고, 그 세션의 첫 체크포인트 오토세이브가 `SaveGame->SlotName` 그대로 `AsyncSaveGameToSlot` 을 호출하므로(`:293`) 아직 살아 있던 원본 파일이 빈 세이브로 덮인다. 백업본이 없으므로 복구 수단이 없다. 빈 슬롯 폴백 자체는 "파일 없이도 사망 리스폰이 월드 리로드에 의존한다"는 의도된 설계지만(헤더 `WxSaveGameSubsystem.h:45`), 그 의도가 다루는 것은 부재 케이스뿐이다. 엔진 업그레이드가 잦은 개발 중에 가장 밟기 쉬운 지뢰다.
- **제안**: `DoesSaveFileExist` 로 부재와 판독 실패를 가른다. 판독 실패면 Error 로그를 남기고 원본을 보존한다 — 최소한 그 슬롯에 대한 쓰기를 이 세션 동안 막거나, 폴백 전에 `.bak` 로 밀어 둔다.
- **확신도**: 중간(폴백 자체는 의도된 설계이나, 손상 케이스까지 같은 처리로 접은 것은 미고려로 보인다)

### 2. 🟡 `FlushPlayerStats` 가 캡처에 실패하면 직전까지 저장돼 있던 스탯까지 지운다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:193-195`, `:200-206`
- **범주**: 버그/정확성
- **문제**: `PlayerStats.Reset()` 을 먼저 부르고 `CapturePlayerStats` 가 아무것도 담지 못하면 `bHasPlayerStats` 가 `false` 로 떨어져 이전 스냅샷이 통째로 사라진다. `CapturePlayerStats` 는 ASC 를 못 찾으면 조용히 return 하는데(`:202-206`), `GetAbilitySystemComponentFromActor` 는 `IAbilitySystemInterface` 를 구현하지 않은 폰에서 null 을 답한다. 플레이어가 ASC 없는 폰(탈것·터렛·연출용 폰)에 빙의한 상태에서 체크포인트 오토세이브가 걸리면 그 순간 세이브의 스탯이 비고, 이후 로드는 데이터테이블 기본 스탯으로 돌아간다 — 남는 로그는 `"어트리뷰트 0개 캡처"`(`:197`)뿐이다. 같은 함수의 폰 부재 경로(`:186-191`)와 `FlushPlayerTransform`(`:161-166`)은 "이전 캡처를 보존한다"는 반대 규약을 지키고 있어 비대칭이다.
- **제안**: 로컬 `TMap` 에 캡처한 뒤 `Num() > 0` 일 때만 `SaveGame->PlayerStats` 로 커밋한다(비면 이전 값과 `bHasPlayerStats` 를 유지하고 Warning).
- **확신도**: 중간(현재 플레이어 폰은 ASC 를 기본 서브오브젝트로 들고 있어 통상 경로에선 재현되지 않는다)

### 3. 🟡 `bSaveInProgress`/`OnSaveCompleted` 가 "저장은 한 번에 하나" 전제라 중첩 시 신호가 어긋난다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:150`, `:278-281`, `:312-318`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:48-58`
- **범주**: 버그/정확성
- **문제**: 진행 상태가 bool 하나이고 완료 통지는 "발화하면서 스스로 비우는" 멀티캐스트 하나다. 요청을 세지 않으므로 저장 A 의 기록이 끝나기 전에 저장 B 가 걸리면(오토세이브 진행 중 메뉴 저장, 체크포인트 연속 통과 등) A 의 완료 콜백이 `bSaveInProgress` 를 내리고 `Broadcast()` 해 B 의 대기자까지 함께 깨운다. `FWxStateTreeTask_SaveGame` 은 `bShouldCallTick = false` 라 이 신호에만 의존하므로, B 를 요청한 상태가 자기 기록이 끝나기 전에 `Succeeded` 로 빠진다. 데이터가 섞이지는 않는다(엔진이 호출 시점에 동기 직렬화하고 쓰기는 파이프로 직렬화한다 — 「검토 범위」 참고). 부수적으로 `Broadcast()` 뒤에 `Clear()` 를 부르므로(`:316-317`), 브로드캐스트 도중 새로 등록한 청자는 곧바로 지워져 영영 신호를 못 받는다 — `OnSaveCompleted` 가 public 인 이상 남는 함정이다.
- **제안**: 진행 카운터(또는 요청별 토큰)로 바꿔 콜백이 자기 요청만 차감하게 한다. 최소한 `FinishSaveInProgress` 에서 델리게이트를 `MoveTemp` 으로 로컬에 옮긴 뒤 브로드캐스트해 중첩 등록이 지워지지 않게 한다.
- **확신도**: 높음(메커니즘은 확실, 영향 범위는 중첩 빈도에 달림)

### 4. 🟡 월드 서브시스템이 없으면 플러시 없이 슬롯을 덮어쓴다 — 경고조차 없다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:152-170`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:42-52`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp:68-74`
- **범주**: 설계/구조
- **문제**: `UWxSaveWorldSubsystem::ShouldCreateSubsystem` 이 `NM_Client` 월드를 제외하므로, 클라이언트에서 `SaveToFile` 이 불리면 `else` 분기로 빠져 **라이브 상태 플러시 없이** 인메모리 SaveGame(클라에선 `Initialize` 가 만든 빈 부트스트랩 슬롯)을 그대로 디스크에 쓴다. `UWxSaveLibrary::SaveToFile` 은 권위 게이트가 없는 BP 진입점이라(`HasAuthority` 를 보는 것은 ST 태스크뿐이다) UI 저장 버튼이 클라에서 눌리면 로컬 슬롯 파일이 빈 세이브로 덮인다. 헤더는 이 분기를 "트랜지션 등"으로 설명하지만 실제로 도달 가능한 주 경로는 클라이언트다. 로그가 전혀 없어 사후 진단도 어렵다.
- **제안**: `else` 분기에 Warning 을 남기고, 스탠드얼론 전제를 유지할 것이라면 `SaveToFile` 진입에서 권위/월드 서브시스템 부재를 판정해 중단한다(그러면 `UWxSaveLibrary` 도 같은 게이트를 탄다).
- **확신도**: 중간(스탠드얼론 싱글 전제라면 도달하지 않는 의도된 단순화일 수 있음)

### 5. 🟡 월드 서브시스템 전역에 SaveGame 조회 전문(前文)이 8회 복제돼 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:106-113`, `:123-131`, `:145-152`, `:176-183`, `:423-429`, `:455-461`, `:479-485`, `:509-514`
- **범주**: 중복/복잡도
- **문제**: `GetWorld() → GetGameInstance() → GetSubsystem<UWxSaveGameSubsystem>() → GetSaveGame()` 사슬과 null 가드가 8개 함수에 그대로 반복된다. 사본마다 실패 처리도 갈린다 — `FlushMapTravelData`/`FlushSavableActors` 는 경고를 남기고 나머지 6곳은 조용히 return 한다. 새 핸들러를 추가할 때 어느 사본을 복사하느냐로 진단 가능성이 달라지고, 조회 규칙이 바뀌면 8곳을 고쳐야 한다. 형제 파일 `WxSaveLibrary.cpp:10-33` 은 같은 사슬을 이미 헬퍼로 접어 두었다.
- **제안**: private `UWxSaveGameSubsystem* GetGameSubsystem() const` / `UWxSaveGame* GetActiveSaveGame() const` 두 헬퍼로 접고 로그 정책도 그 안에서 통일한다.
- **확신도**: 높음

### 6. 🟢 델리게이트 콜백 `ContinueSaveToFileToDisk` 의 `Handle` prefix 누락과 불필요한 람다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:163-165`, `:292-309`
- **범주**: 규칙 위반
- **문제**: (a) `ContinueSaveToFileToDisk` 는 `FOnSaveFlushComplete::FDelegate::CreateUObject` 로 바인딩되는 콜백인데 `Handle` prefix 가 없다(코딩 규칙 4). (b) `AsyncSaveGameToSlot` 완료 콜백이 `CreateLambda` + `TWeakObjectPtr` 수동 캡처로 작성돼 있는데, `FAsyncSaveGameToSlotDelegate` 는 일반 델리게이트라 `CreateUObject` 로 멤버 함수를 물리면 약참조 수명 처리를 엔진이 대신한다 — 람다가 필요한 자리가 아니다(코딩 규칙 3). 같은 파일 `:12-26` 의 콘솔 명령 람다(바인딩할 UObject 자체가 없다)와 `WxStateTreeTask_SaveGame.cpp:55` 의 약 실행 컨텍스트 람다(USTRUCT 태스크라 `CreateUObject` 불가)는 대안이 없어 해당하지 않는다.
- **제안**: `ContinueSaveToFileToDisk` → `HandleSaveFlushComplete` 로 개명하고, 비동기 기록 콜백은 `HandleAsyncSaveComplete(const FString&, int32, bool)` 멤버로 빼 `CreateUObject(this, ...)` 로 바인딩한다.
- **확신도**: 높음

### 7. 🟢 역직렬화 실패를 감지하지 않고 복원 성공으로 보고한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:373-384`, `:386-408`, `:410-413`
- **범주**: 버그/정확성
- **문제**: `FMemoryReader` 가 스트림 끝을 넘어 읽거나 타입이 맞지 않으면 `ArIsError` 가 서는데, 액터·컴포넌트 어느 리더도 `IsError()` 를 확인하지 않는다(모듈 전체에 `IsError` 호출 0건). 반쯤 덮인 상태 그대로 `OnSaveRestored()` 를 부르고 `true`(복원 성공)를 반환하므로 로그 집계에도 정상으로 잡힌다. 태그 기반 프로퍼티 직렬화라 `UPROPERTY(SaveGame)` 필드의 추가·제거·순서 변경은 안전하지만(그래서 이 설계가 성립한다), 필드 **타입** 변경이나 레코드 손상은 이 경로로 조용히 흘러간다.
- **제안**: 각 `Serialize` 뒤에 `MemReader.IsError()` 를 확인해 Warning 을 남기고, 최소한 액터 본체 실패 시엔 `OnSaveRestored()` 호출을 건너뛴다.
- **확신도**: 중간(현실적 트리거가 드묾)

### 8. 🟢 컴포넌트 레코드를 이름 기준으로 전량 기록해 낭비와 무경고 실패를 함께 낳는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:308-327`, `:386-408`
- **범주**: 성능/안전, 설계/구조
- **문제**: (a) `CaptureActor` 는 savable 액터의 **모든** 컴포넌트를 조건 없이 순회해 `ComponentData` 항목을 만든다. `UPROPERTY(SaveGame)` 필드가 하나도 없는 컴포넌트(메시·콜리전·오디오 등 대부분)도 프록시 아카이브를 세우고 종결자만 담긴 바이트 배열 + `FName` 키 항목을 세이브 파일에 남긴다 — 레코드 크기와 저장 비용이 "저장할 데이터 양"이 아니라 "savable 액터의 총 컴포넌트 수"에 비례한다. (b) 복원은 컴포넌트 `FName` 일치로만 이뤄지고(`:393`) 짝을 못 찾은 레코드는 아무 로그 없이 건너뛴다. 에디터 배치 컴포넌트는 이름이 안정적이라 지금은 맞아떨어지지만, 런타임에 `NewObject` 로 만든 컴포넌트는 생성 순서에 따라 이름이 갈려 조용히 복원되지 않는다. "컴포넌트 이름이 세션을 넘어 안정적이어야 한다"는 계약이 `IWxSavable` 주석에도 README 에도 없다.
- **제안**: 클래스 단위로 `CPF_SaveGame` 프로퍼티 보유 여부를 1회 판정해 캐시하고 해당 컴포넌트만 기록한다. 복원 시 짝 없는 레코드는 Verbose 로그라도 남겨 이름 불일치가 드러나게 하고, 이름 안정성 요구를 `IWxSavable` 주석에 명시한다.
- **확신도**: 높음(사실 관계), 중간(영향 — 현재 savable 액터 수가 적어 체감은 작다)

### 9. 🟢 `UWxPlayerSpawnComponent` 의 캐치업이 멱등하지 않고 빙의 구독이 해제되지 않는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:19-34`, `:37-43`, `:54`, `:78-88`, `:91-99`
- **범주**: 버그/정확성
- **문제**: `OnRegister` 는 `PostLoginHandle.IsValid()` 로 중복 구독만 막는데 `OnUnregister` 가 그 핸들을 `Reset()` 하므로 재등록 시 가드가 풀린다. 재등록 시점에 오너 PC 에 `PlayerState` 가 있으면 캐치업이 `HandleGameModePostLogin` 을 다시 실행해 — (a) `APlayerStart` 마커를 하나 더 스폰하고 `StartSpot` 을 교체하며(이전 마커는 월드에 남아 `ChoosePlayerStart` 후보로 떠돈다), (b) `SetInitialLocationAndRotation` 으로 플레이 중인 PC 를 저장 좌표로 되돌리고, (c) `OnPossessedPawnChanged` 에 중복 바인딩을 건다(`AddDynamic` 은 `Add`, 중복 제거는 `AddUniqueDynamic` 만 한다). 또 이 구독은 `OnUnregister` 에서 해제되지 않고 **모든** 빙의 변경에 반응하므로, 게임 중 재빙의 경로가 생기면(탈것 하차 등) 그때마다 마지막 저장 시점 스탯이 현재 값 위에 덮인다 — `bHasPlayerStats` 는 한 번 서면 내려가지 않아 영구적으로 열린 문이다.
- **제안**: "이미 셋업했다" 표식(또는 스폰한 마커의 약참조)으로 캐치업을 1회로 묶고, `OnUnregister` 에서 `RemoveDynamic` 해 등록/해제를 대칭으로 만든다. 스탯 복원이 "로드/부활 직후 1회"가 의도라면 첫 적용 후 구독을 끊는다.
- **확신도**: 중간(경로는 확실, 현재 ControllerComponent 재등록·플레이어 재빙의 빈도는 낮다 — C++ 에 플레이어 `Possess` 호출부가 없다)

### 10. 🟢 월드 초기화 복원이 액터당 `FindSavable` 을 두 번 돈다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:434-442` (재호출은 `RestoreActor` 내부 `:340`)
- **범주**: 중복/복잡도, 성능/안전
- **문제**: 로그용 `SavableCount` 를 세려고 `if (FindSavable(Actor))` 를 돌린 뒤, 그 안에서 `RestoreActor` 가 같은 `FindSavable` 을 다시 호출한다. `FindSavable` 은 `Cast` 실패 시 `FindComponentByInterface` 로 액터의 전 컴포넌트를 훑으므로(`:101`) 공짜가 아니고, 이 루프의 대상은 월드의 모든 액터다. 같은 성격의 전 액터 순회가 저장할 때마다(`FlushSavableActors:135-138`), 맵을 뜰 때마다(`HandleWorldBeginTearDown:526`) 반복된다 — 오픈월드 규모가 커지면 체크포인트 저장 히치로 드러날 자리다.
- **제안**: 최소한 이중 호출은 없앤다(`RestoreActor` 가 "savable 이었는가"를 out 파라미터로 알려주게). 액터 수가 커지면 `IWxSavable` 액터를 BeginPlay/EndPlay 에 등록하는 레지스트리로 전 액터 순회 자체를 걷어내는 것을 검토한다.
- **확신도**: 높음(중복), 중간(성능 — 실제 액터 수에 달림)

### 11. 🟢 어트리뷰트 스냅샷 키가 프로퍼티 이름뿐이라 AttributeSet 간 충돌 여지가 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:209-227`, `:249-271`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h:83`
- **범주**: 설계/구조
- **문제**: `CapturePlayerStats` 는 모든 `SpawnedAttributes` 를 순회하며 `It->GetFName()` 을 평면 `TMap<FName, float>` 키로 쓴다. 서로 다른 AttributeSet 이 같은 이름의 어트리뷰트를 가지면 나중 것이 앞의 것을 덮고, `ApplyPlayerStats` 는 그 하나의 값을 두 세트 모두에 적용한다. 플레이어 AttributeSet 이 `UWxCombatAttributeSet` 하나뿐인 현재는 발생하지 않지만, 순회 구조 자체가 다중 세트를 전제하고 있어 세트가 늘어나는 순간 조용히 어긋난다(세이브 포맷이라 나중에 고치려면 마이그레이션이 필요하다).
- **제안**: 키를 `"<AttributeSetClass>.<PropertyName>"` 로 승격한다. 당장 미룬다면 단일 세트 전제를 `PlayerStats` 주석에 명시한다.
- **확신도**: 낮음(의도된 단순화일 수 있음)

### 12. 🟢 `GameplayTags` 는 쓰이지 않는 의존, `GameplayAbilities` 는 private 로 충분하다
- **위치**: `Plugins/WxSave/Source/WxSave/WxSave.Build.cs:16-17`
- **범주**: 설계/구조
- **문제**: 모듈 전체에 `GameplayTag` 식별자가 한 번도 등장하지 않는다(소스 전량 검색 기준 Build.cs 항목 외 0건). `GameplayAbilities` 도 `WxSaveWorldSubsystem.cpp` 에서만 쓰이고 public 헤더에 노출된 GAS 타입은 없다(주석 한 줄이 전부). 지금은 GAS 가 GameplayTags 를 끌고 오므로 빌드에 영향은 없다.
- **제안**: `GameplayTags` 제거, `GameplayAbilities` 는 `PrivateDependencyModuleNames` 로 이동. (`StateTreeModule` 은 public 헤더 `WxStateTreeTask_SaveGame.h` 가 `StateTreeTaskBase.h` 를 포함하므로 public 유지가 맞다.)
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/WxSave.uplugin`, `Plugins/WxSave/README.md`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`(`OnSaveRestored` 만)
- **엔진 소스와 대조해 기각한 항목**(재조사 낭비를 막기 위해 남긴다):
  - `UGameplayStatics::AsyncSaveGameToSlot` 은 호출 스레드에서 `SaveGameToMemory` 로 동기 직렬화한 뒤 바이트만 비동기로 쓰고, 조기 실패(슬롯명 공백·직렬화 실패) 시에도 델리게이트를 반드시 실행한다(`GameplayStatics.cpp:2403-2426`). 게다가 실제 쓰기는 `ISaveGameSystem::SaveGameAsync` 가 `AsyncTaskPipe.Launch` 로 직렬화한다(`SaveGameSystem.cpp:40-68`). 따라서 기록 대기 중 SaveGame 을 수정해도 레이스가 없고, 중첩 저장이 파일을 뒤섞거나 `bSaveInProgress` 가 영구히 걸려 ST 태스크가 갇히는 경로도 없다 — 발견 3번의 피해가 "신호 어긋남"에 그치는 근거다.
  - `CaptureActor`/`RestoreActor` 의 커스텀 버전 헤더 처리는 정상이다. `FArchiveProxy` 생성자가 내부 archive 상태를 복사한 뒤 프록시를 링크하므로(`ArchiveProxy.cpp:9-14`), 리더에서 프록시 생성 **전에** `SetUEVer`/`SetCustomVersions` 를 부르는 현재 순서(`WxSaveWorldSubsystem.cpp:376-381`)가 맞다.
  - 패키지 경로만 담는 `FSoftObjectPath` 왕복(`FlushMapTravelData` → `TravelFromSaveFile`/`TryGetPlayerTransform`)은 성립한다. `FSoftObjectPath::SetPath` 가 구분자 없는 경로를 "패키지명만"으로 받아들여 `AssetPath` 에 패키지명만 채우므로(`SoftObjectPath.cpp:220-227`) `IsNull()` 은 false 이고 맵 일치 비교가 정상 동작한다.
  - `StartSpot` 주입이 엔진 스폰 경로에 먹히는 것도 확인했다. `AWxGameMode` 는 `AGameModeBase` 파생이고 `AGameModeBase::ShouldSpawnAtStartSpot` 은 `StartSpot` 이 있으면 무조건 true 를 답해 `ChoosePlayerStart` 보다 우선한다(`GameModeBase.cpp:1142-1144`, `1168-1178`). 다만 이 때문에 마커는 한 번 심으면 그 월드 내내 유효하다 — 발견 9번의 (a)(b) 가 그 부작용이다.
  - `WxStateTreeTask_SaveGame.h:42` 의 헤더 인라인 정의는 코딩 규칙 6 위반처럼 보이나, 같은 파일 13행에 예외 근거가 명시돼 있고 엔진 StateTree 태스크가 전부 같은 형태라 프로젝트 관례로 보아 발견으로 올리지 않았다.
  - `PlayerTransform` 의 Identity sentinel(별도 bool 없음)은 `WxSaveGame.h:85-92` 에 근거가 명시된 설계 결정이라 발견으로 올리지 않았다(원점 정확히 위에서 저장하면 "미설정"으로 읽히는 이론적 구멍은 남는다).
  - `HandleLevelAddedToWorld` 의 복원이 스트리밍-인 셀에서 액터 `BeginPlay` **이후**에 걸리는 순서 차이는 소비자가 이미 흡수하고 있다 — `UWxGimmickStateTreeComponent::OnSaveRestored` 가 `IsRunning()` 이면 `RestartLogic()` 으로 다시 연다(`WxGimmickStateTreeComponent.cpp:160-168`).
- **미검토 / 한계**: `ShouldCreateSubsystem` 의 `NM_Client` 게이트가 월드 서브시스템 생성 시점에 실제로 클라이언트를 걸러내는지(넷드라이버 부착 순서)는 이번 패스에서 엔진 소스로 확인하지 않았다 — 발견 4번은 그 게이트가 도는 것을 전제로 한 지적이다. `IWxSavable` 구현체(`AWxSpawner`·`UWxGimmickStateTreeComponent`)는 복원 멱등성 판단에 필요한 만큼만 보았고 그 자체는 리뷰하지 않았다. 실제 세이브 파일 왕복·이기종 빌드 간 레코드 호환은 빌드/에디터 실행 없이 검증할 수 없어 정적 분석에 그쳤고, `UWxSaveLibrary` 를 호출하는 BP/WBP 측 사용 패턴도 범위 밖이다.

---
*문서 기준 커밋 `ce04ce1f` · 리뷰일 2026-08-21 · 소스 13파일 — `/module-review`로 갱신*
