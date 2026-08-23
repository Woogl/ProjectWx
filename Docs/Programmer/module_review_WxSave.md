# WxSave — 코드 리뷰

> 13개 파일의 작고 밀도 높은 모듈이다. 프로젝트 규칙 준수도가 높고(`WxCore` 외 Wx 의존 없음, 전 파일 Copyright 헤더, `Super::` 호출 전량 준수, `BlueprintCallable` 은 BP 라이브러리에만), 직렬화 버전 헤더·트래블 가드처럼 틀리기 쉬운 자리엔 근거 주석이 붙어 있다. 직전 리뷰 이후 저장 대상 계약이 액터 전용으로 정리되고(`313c9ba5`) 기본값 스킵이 들어와(`bd689a19`) 컴포넌트 전수 기록 문제와 전 액터 순회 비용이 크게 줄었지만, 남은 문제는 여전히 크래시가 아니라 "실패·중첩·비권위 경로에서 세이브가 조용히 비거나 완료 신호가 어긋나는" 쪽에 몰려 있고 그중 하나는 진행 상황을 영구히 잃는다. 이번 리뷰는 소스 13개 전부를 읽고 직렬화(`ShouldSave`/`CaptureActor`/`RestoreActor`)·슬롯 I/O·저장 완료 신호·플레이어 스냅샷·스폰 주입 타이밍을 UE 5.8 엔진 소스와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 4 |
| 🟢 사소 | 8 |

## 결과

### 1. 🔴 판독 실패한 세이브 파일을 빈 슬롯으로 대체한 뒤 같은 슬롯에 되쓴다 — 진행 상황 영구 소실
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:79-92`, `:292`
- **범주**: 버그/정확성
- **문제**: `LoadFromFile` 은 `LoadGameFromSlot` 이 null 을 답하면 무조건 `StartNewSaveFile(TargetSlot, ...)` 로 **같은 슬롯 이름의 빈 SaveGame** 을 활성 슬롯에 앉힌다(`:90`). 그런데 엔진은 부재·손상·클래스 소실을 모두 nullptr 하나로 접는다 — `UGameplayStatics::LoadGameFromSlot` 은 `LoadDataFromSlot` 실패(파일 부재·I/O 실패)에 nullptr 을 답하고, `LoadGameFromMemory` 는 빈 버퍼이거나 헤더의 SaveGame 클래스를 못 찾으면 역시 nullptr 을 답한다(`GameplayStatics.cpp:2457-2489`, `:2536-2546`). 여기에 `Cast<UWxSaveGame>` 실패까지 같은 분기로 들어온다. 즉 "파일이 없다"와 "파일은 있는데 못 읽는다"가 구분되지 않는다. 후자일 때 남는 흔적은 `"파일 없음/손상"` Log 한 줄(`:91`)뿐이고, 그 세션의 첫 체크포인트 오토세이브가 `SaveGame->SlotName` 그대로 `AsyncSaveGameToSlot` 을 호출하므로(`:292`) 아직 살아 있던 원본 파일이 빈 세이브로 덮인다. 백업본이 없어 복구 수단이 없다. 빈 슬롯 폴백 자체는 "파일 없이도 사망 리스폰이 월드 리로드에 의존한다"는 의도된 설계지만(`WxSaveGameSubsystem.h:45`), 그 의도가 다루는 것은 부재 케이스뿐이다. 엔진 업그레이드·클래스 리네임이 잦은 개발 중에 가장 밟기 쉬운 지뢰다.
- **제안**: `DoesSaveFileExist` 로 부재와 판독 실패를 가른다. 판독 실패면 Error 로그를 남기고 원본을 보존한다 — 그 슬롯에 대한 쓰기를 이 세션 동안 막거나, 폴백 전에 `.bak` 로 밀어 둔다.
- **확신도**: 중간(폴백 자체는 의도된 설계이나, 손상 케이스까지 같은 처리로 접은 것은 미고려로 보인다)

### 2. 🟡 `bSaveInProgress`/`OnSaveCompleted` 가 "저장은 한 번에 하나" 전제라 중첩 시 신호가 어긋난다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:149`, `:311-317`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:247-257`
- **범주**: 버그/정확성
- **문제**: 진행 상태가 bool 하나이고 완료 통지는 "발화하면서 스스로 비우는" 멀티캐스트 하나다. 요청을 세지 않으므로 저장 A 의 기록이 끝나기 전에 저장 B 가 걸리면(오토세이브 진행 중 메뉴 저장, 체크포인트 연속 통과 등) A 의 완료 콜백이 `bSaveInProgress` 를 내리고 `Broadcast()` 해 B 의 대기자까지 함께 깨운다. `FWxStateTreeTask_SaveGame` 은 `bShouldCallTick = false` 라 이 신호에만 의존하므로(`WxStateTreeTask_SaveGame.cpp:254-257`), B 를 요청한 상태가 자기 기록이 끝나기 전에 `Succeeded` 로 빠진다. 데이터가 섞이지는 않는다(엔진이 호출 시점에 동기 직렬화하고 쓰기는 파이프로 직렬화한다 — 「검토 범위」 참고). 부수적으로 `Broadcast()` 뒤에 `Clear()` 를 부르므로(`:315-316`), 브로드캐스트 도중 새로 등록한 청자는 곧바로 지워져 영영 신호를 못 받는다 — 그 대기자가 ST 태스크면 `Running` 에서 갇힌다.
- **제안**: 진행 카운터(또는 요청별 토큰)로 바꿔 콜백이 자기 요청만 차감하게 한다. 최소한 `FinishSaveInProgress` 에서 델리게이트를 `MoveTemp` 으로 로컬에 옮긴 뒤 브로드캐스트해 중첩 등록이 지워지지 않게 한다.
- **확신도**: 높음(메커니즘은 확실, 영향 범위는 중첩 빈도에 달림)

### 3. 🟡 비권위 경로에 게이트가 없다 — 클라에서 부르면 빈 세이브를 쓰고 세션에서 이탈한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:133-170`, `:102-131`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:42-52`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp:166-180`
- **범주**: 설계/구조 (권위 모델)
- **문제**: `UWxSaveWorldSubsystem::ShouldCreateSubsystem` 이 `NM_Client` 월드를 제외하므로(이 판정은 넷드라이버 부착 전에도 성립한다 — 「검토 범위」 참고), 클라이언트에서 `SaveToFile` 이 불리면 `else` 분기(`:168`)로 빠져 **라이브 상태 플러시 없이** 인메모리 SaveGame(클라에선 `Initialize` 가 만든 빈 부트스트랩 슬롯)을 그대로 디스크에 쓴다. 로그조차 없다. `TravelFromSaveFile` 은 더 나쁘다 — 클라엔 `AuthGameMode` 가 없어 `UWorld::ServerTravel` 이 조기 반환하지 않고 `NextURL` + `NextSwitchCountdown = 0` 을 세팅해 그 클라를 세션에서 떼어내 로컬 맵으로 보낸다. `UWxSaveLibrary` 의 `SaveToFile`/`LoadFromFile`/`TravelFromSaveFile` 은 권위 게이트가 없는 BP 진입점이고(`HasAuthority` 를 보는 것은 ST 태스크뿐이다), 모듈 밖 C++ 호출자가 아예 없어 UI 슬롯 흐름이 이 파사드를 그대로 쓴다.
- **제안**: `SaveToFile`/`LoadFromFile`/`TravelFromSaveFile` 진입에서 권위(또는 월드 서브시스템 부재)를 판정해 Warning 과 함께 중단한다. 서브시스템에 두면 `UWxSaveLibrary` 도 자동으로 같은 게이트를 탄다.
- **확신도**: 중간(스탠드얼론 싱글 전제라면 도달하지 않는 의도된 단순화일 수 있음)

### 4. 🟡 `FlushPlayerStats` 가 캡처에 실패하면 직전까지 저장돼 있던 스탯까지 지운다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:194-196`, `:201-207`
- **범주**: 버그/정확성
- **문제**: `PlayerStats.Reset()` 을 먼저 부르고(`:194`) `CapturePlayerStats` 가 아무것도 담지 못하면 `bHasPlayerStats` 가 `false` 로 떨어져 이전 스냅샷이 통째로 사라진다. `CapturePlayerStats` 는 ASC 를 못 찾으면 조용히 return 하는데(`:204-207`), `GetAbilitySystemComponentFromActor` 는 `IAbilitySystemInterface` 를 구현하지 않은 폰에서 null 을 답한다. 플레이어가 ASC 없는 폰(탈것·터렛·연출용 폰)에 빙의한 상태에서 체크포인트 오토세이브가 걸리면 그 순간 세이브의 스탯이 비고, 이후 로드는 데이터테이블 기본 스탯으로 돌아간다 — 남는 로그는 `"어트리뷰트 0개 캡처"`(`:198`)뿐이다. 같은 함수의 폰 부재 경로(`:188-192`)와 `FlushPlayerTransform`(`:167-170`)은 "이전 캡처를 보존한다"는 반대 규약을 지키고 있어 비대칭이다.
- **제안**: 로컬 `TMap` 에 캡처한 뒤 `Num() > 0` 일 때만 `SaveGame->PlayerStats` 로 커밋한다(비면 이전 값과 `bHasPlayerStats` 를 유지하고 Warning).
- **확신도**: 중간(현재 플레이어 폰은 ASC 를 기본 서브오브젝트로 들고 있어 통상 경로에선 재현되지 않는다)

### 5. 🟡 `RequestSaveFlush` 가 `ResumeTransform` 을 원시 포인터로 받는다 — 스스로 열어 둔 비동기 seam 과 충돌
- **위치**: `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h:32-41`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:26-40`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:240-244`
- **범주**: 설계/구조 (객체 수명)
- **문제**: 완료 델리게이트(`FOnSaveFlushComplete`)는 "비동기 작업이 생길 때 지연 완료로 되돌릴 seam" 으로 일부러 남겨 둔 것이라고 헤더가 명시한다(`WxSaveWorldSubsystem.h:32`). 그런데 같은 함수가 재개 지점을 `const FTransform*` 로 받고(`:41`), 실제 인자는 호출부의 스택 지역이다 — `FWxStateTreeTask_SaveGame::EnterState` 의 로컬 `ResumeTransform`(`WxStateTreeTask_SaveGame.cpp:241`)이 `SaveToFile` → `RequestSaveFlush` → `FlushPlayerTransform` 으로 그대로 흘러간다. 지금은 전 구간이 동기라 안전하지만, seam 을 실제로 쓰는 순간(플러시 한 단계라도 프레임을 넘기는 순간) 이 포인터는 댕글링이 되고 증상은 "재개 지점이 가끔 쓰레기 값" 이라는 최악의 형태로 나타난다. seam 을 열어 둔 설계와 포인터 전달이 서로 모순이다.
- **제안**: `TOptional<FTransform>`(또는 값 + bool)으로 바꿔 값 소유로 만든다. 호출 사슬이 짧아 변경 비용이 거의 없다.
- **확신도**: 높음(현재는 동작하지만, 명시된 확장 방향과 충돌한다)

### 6. 🟢 역직렬화 실패를 감지하지 않고 복원 성공으로 보고한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:412-424`, `:426-448`, `:450`
- **범주**: 버그/정확성
- **문제**: `FMemoryReader` 가 스트림 끝을 넘어 읽거나 타입이 맞지 않으면 `ArIsError` 가 서는데, 액터·컴포넌트 어느 리더도 `IsError()` 를 확인하지 않는다(모듈 전체에 `IsError` 호출 0건). 반쯤 덮인 상태 그대로 `OnSaveRestored()` 를 부르고(`:450`) `true`(복원 성공)를 반환하므로 로그 집계에도 정상으로 잡힌다. 태그 기반 프로퍼티 직렬화라 `UPROPERTY(SaveGame)` 필드의 추가·제거·순서 변경은 안전하지만(그래서 이 설계가 성립한다), 필드 **타입** 변경이나 레코드 손상은 이 경로로 조용히 흘러간다.
- **제안**: 각 `Serialize` 뒤에 `MemReader.IsError()` 를 확인해 Warning 을 남기고, 최소한 액터 본체 실패 시엔 `OnSaveRestored()` 호출을 건너뛴다.
- **확신도**: 중간(현실적 트리거가 드묾)

### 7. 🟢 컴포넌트 레코드는 이름으로만 짝을 찾고, 못 찾으면 아무 흔적 없이 사라진다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:332-350`, `:426-448`
- **범주**: 설계/구조
- **문제**: 캡처는 컴포넌트 `FName` 을 키로 쓰고(`:339`), 복원은 같은 이름의 컴포넌트를 찾아 붙인다(`:433`). 짝을 못 찾은 레코드는 로그 한 줄 없이 `continue` 로 버려진다(`:434-437`). 에디터 배치·`CreateDefaultSubobject` 컴포넌트는 이름이 안정적이라 현재 유일한 대상인 `UWxDeviceStateTreeComponent` 는 문제없지만, 런타임에 `NewObject` 로 만든 컴포넌트는 생성 순서에 따라 이름이 갈려 조용히 복원되지 않는다. "컴포넌트 이름이 세션을 넘어 안정적이어야 한다"는 계약이 `IWxSavable` 주석(`Plugins/WxCore/Source/WxCore/Public/WxSavable.h:12`)에도 README 에도 없다.
- **제안**: 짝 없는 레코드는 Verbose 로그라도 남겨 이름 불일치가 드러나게 하고, 이름 안정성 요구를 `IWxSavable` 주석에 명시한다.
- **확신도**: 높음(사실 관계), 중간(영향 — 현재 savable 컴포넌트가 하나뿐이라 체감은 작다)

### 8. 🟢 `UWxPlayerSpawnComponent` 의 캐치업이 멱등하지 않고 빙의 구독이 해제되지 않는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:20`, `:36-42`, `:53`, `:77-87`
- **범주**: 버그/정확성
- **문제**: `OnRegister` 는 `PostLoginHandle.IsValid()` 로 중복 구독만 막는데(`:20`) `OnUnregister` 가 그 핸들을 `Reset()` 하므로(`:39`) 재등록 시 가드가 풀린다. 재등록 시점에 오너 PC 에 `PlayerState` 가 있으면 캐치업이 `HandleGameModePostLogin` 을 다시 실행해 — (a) `APlayerStart` 마커를 하나 더 스폰하고 `StartSpot` 을 교체하며(`:77-84`, 이전 마커는 월드에 남아 `ChoosePlayerStart` 후보로 떠돈다), (b) `SetInitialLocationAndRotation` 으로 플레이 중인 PC 를 저장 좌표로 되돌리고(`:87`), (c) `OnPossessedPawnChanged` 에 중복 바인딩을 건다(`:53` — `AddDynamic` 은 `FMulticastScriptDelegate::Add` 로 내려가 중복을 거르지 않는다. 거르는 것은 `AddUnique` 뿐이다: `ScriptDelegates.h:1222`, `:1243`). 또 이 구독은 `OnUnregister` 에서 해제되지 않고 **모든** 빙의 변경에 반응하므로, 게임 중 재빙의 경로가 생기면(탈것 하차, 맵 리로드 없는 부활 등) 그때마다 마지막 저장 시점 스탯이 현재 값 위에 덮인다 — `bHasPlayerStats` 는 한 번 서면 내려가지 않아 영구적으로 열린 문이다.
- **제안**: "이미 셋업했다" 표식(또는 스폰한 마커의 약참조)으로 캐치업을 1회로 묶고, `OnUnregister` 에서 `RemoveDynamic` 해 등록/해제를 대칭으로 만든다. 스탯 복원이 "로드/부활 직후 1회"가 의도라면 첫 적용 후 구독을 끊는다.
- **확신도**: 중간(경로는 확실, 현재 ControllerComponent 재등록·플레이어 재빙의 빈도는 낮다 — C++ 에 플레이어 `Possess` 호출부가 없다)

### 9. 🟢 델리게이트 콜백 `ContinueSaveToFileToDisk` 의 `Handle` prefix 누락과 불필요한 람다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:163`, `:282`, `:291-308`
- **범주**: 규칙 위반
- **문제**: (a) `ContinueSaveToFileToDisk` 는 `FOnSaveFlushComplete::FDelegate::CreateUObject` 로 바인딩되는 콜백인데(`:163`) `Handle` prefix 가 없다(코딩 규칙 4). (b) `AsyncSaveGameToSlot` 완료 콜백이 `CreateLambda` + `TWeakObjectPtr` 수동 캡처로 작성돼 있는데(`:291-308`), `FAsyncSaveGameToSlotDelegate` 는 일반 델리게이트라 `CreateUObject` 로 멤버 함수를 물리면 약참조 수명 처리를 엔진이 대신한다 — 람다가 필요한 자리가 아니다(코딩 규칙 3). 같은 파일 `:12-26` 의 콘솔 명령 람다(바인딩할 UObject 자체가 없다)와 `WxStateTreeTask_SaveGame.cpp:254` 의 약 실행 컨텍스트 람다(USTRUCT 태스크라 `CreateUObject` 불가)는 대안이 없어 해당하지 않는다.
- **제안**: `ContinueSaveToFileToDisk` → `HandleSaveFlushComplete` 로 개명하고, 비동기 기록 콜백은 `HandleAsyncSaveComplete(const FString&, int32, bool)` 멤버로 빼 `CreateUObject(this, ...)` 로 바인딩한다.
- **확신도**: 높음

### 10. 🟢 어트리뷰트 스냅샷 키가 프로퍼티 이름뿐이라 AttributeSet 간 충돌 여지가 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:217-226`, `:249-271`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h:82`
- **범주**: 설계/구조
- **문제**: `CapturePlayerStats` 는 모든 `SpawnedAttributes` 를 순회하며 `It->GetFName()` 을 평면 `TMap<FName, float>` 키로 쓴다(`:225`). 서로 다른 AttributeSet 이 같은 이름의 어트리뷰트를 가지면 나중 것이 앞의 것을 덮고, `ApplyPlayerStats` 는 그 하나의 값을 두 세트 모두에 적용한다(`:256-270`). 플레이어 AttributeSet 이 `UWxCombatAttributeSet` 하나뿐인 현재는 발생하지 않지만, 순회 구조 자체가 다중 세트를 전제하고 있어 세트가 늘어나는 순간 조용히 어긋난다(세이브 포맷이라 나중에 고치려면 마이그레이션이 필요하다).
- **제안**: 키를 `"<AttributeSetClass>.<PropertyName>"` 로 승격한다. 당장 미룬다면 단일 세트 전제를 `PlayerStats` 주석에 명시한다.
- **확신도**: 낮음(의도된 단순화일 수 있음)

### 11. 🟢 `StartNewSaveFile` 이 빈 슬롯 이름을 검증하지 않는다 — 이후 모든 저장이 조용히 실패
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:45-66`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h:25-27`
- **범주**: 버그/정확성
- **문제**: 슬롯 이름이 그대로 슬롯 정체성이 되는데(`:60`) 빈 문자열을 걸러내지 않는다. `UGameplayStatics::AsyncSaveGameToSlot` 은 `SlotName.Len() > 0` 을 요구하고 아니면 즉시 실패 델리게이트를 실행하므로(`GameplayStatics.cpp:2409`), 빈 이름으로 시작한 세션은 그 뒤 모든 저장이 `"디스크 기록 실패"` Warning 한 줄만 남기고 실패한다. 헤더가 "유효한 이름을 넘겨야 한다"고 계약을 적어 두었지만 이 함수는 BP 진입점(`UWxSaveLibrary::StartNewSaveFile`)이라 기획자 실수의 사거리 안에 있고, `LoadFromFile` 의 빈 슬롯 규칙("활성 슬롯 재사용")과 의미가 달라 헷갈리기도 쉽다.
- **제안**: 빈 `SlotName` 이면 Warning 후 `nullptr` 을 답해 활성 슬롯을 바꾸지 않는다(`SpecificClass` 가 null 일 때와 같은 처리).
- **확신도**: 높음

### 12. 🟢 README 가 폐기된 "컴포넌트 갈래" 를 여전히 저장 대상 계약으로 서술한다
- **위치**: `Plugins/WxSave/README.md:34`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:305`, `:374`
- **범주**: 중복/복잡도 (낡은 계약 서술)
- **문제**: `313c9ba5` 에서 저장 대상 계약이 액터 전용으로 정리돼 `CaptureActor`/`RestoreActor` 는 `Cast<IWxSavable>(Actor)` 만 하고(`:305`, `:374`) 컴포넌트 인터페이스 탐색(`FindSavable`)은 사라졌다. `WxSavable.h:11` 도 "액터만 구현한다"로 갱신됐다. 그런데 README 확장 포인트 항목은 여전히 "액터가 직접 또는 **그 컴포넌트가** `IWxSavable` 을 구현하고 … 컴포넌트 갈래 덕에 호스트 액터는 순수 BP 여도 된다"고 안내한다. 이 문서를 따라 컴포넌트에 인터페이스를 구현하면 컴파일도 되고 경고도 없이 그 액터만 저장에서 통째로 빠진다 — 발견하기 어려운 함정이다.
- **제안**: README 의 해당 문장을 액터 전용 계약으로 고친다(`/readme-writer` 재실행으로도 해소된다).
- **확신도**: 높음

### 13. 🟢 `GameplayTags` 는 쓰이지 않는 의존, `GameplayAbilities` 는 private 로 충분하다
- **위치**: `Plugins/WxSave/Source/WxSave/WxSave.Build.cs:16-17`
- **범주**: 설계/구조
- **문제**: 모듈 전체에 `GameplayTag` 식별자가 한 번도 등장하지 않는다(소스 전량 검색 기준 Build.cs 항목 외 0건). `GameplayAbilities` 도 `WxSaveWorldSubsystem.cpp` 에서만 쓰이고 public 헤더에 노출된 GAS 타입은 없다. 지금은 GAS 가 GameplayTags 를 끌고 오므로 빌드에 영향은 없다.
- **제안**: `GameplayTags` 제거, `GameplayAbilities` 는 `PrivateDependencyModuleNames` 로 이동. (`StateTreeModule` 은 public 헤더 `WxStateTreeTask_SaveGame.h` 가 `StateTreeTaskBase.h` 를 포함하므로 public 유지가 맞다.)
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/WxSave.uplugin`, `Plugins/WxSave/README.md`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h`
- **엔진 대조로 기각·강등한 항목**(재조사 낭비를 막기 위해 남긴다):
  - 직전 리뷰의 🟡 "저장할 때마다 월드 전 액터 순회"와 🟢 "컴포넌트 전량 기록"은 `bd689a19` 의 `ShouldSave`(아키타입 대비 `CPF_SaveGame` 비교, `WxSaveWorldSubsystem.cpp:276-301`)와 `313c9ba5` 의 액터 전용 계약으로 크게 완화됐다. 순회는 여전히 `TActorIterator<AActor>` 전수지만 액터당 비용이 인터페이스 캐스트 하나로 줄어(`:142`, `:305`) 체크포인트 빈도 대비 발견으로 올릴 수준이 아니라 판단해 제외했다.
  - `UGameplayStatics::AsyncSaveGameToSlot` 은 호출 스레드에서 `SaveGameToMemory` 로 동기 직렬화한 뒤 바이트만 비동기로 쓰고, 조기 실패(슬롯명 공백·직렬화 실패) 시에도 델리게이트를 반드시 실행한다(`GameplayStatics.cpp:2403-2426`). 실제 쓰기는 `ISaveGameSystem::SaveGameAsync` 가 파이프로 직렬화한다. 따라서 기록 대기 중 SaveGame 을 수정해도 레이스가 없고, 중첩 저장이 파일을 뒤섞거나 `bSaveInProgress` 가 영구히 걸리는 경로도 없다 — 발견 2번의 피해가 "신호 어긋남"에 그치는 근거다.
  - `ShouldCreateSubsystem` 의 `NM_Client` 게이트는 넷드라이버가 붙기 전에도 성립한다(`UWorld::InternalGetNetMode` 가 URL/PIE 설정에서 유도). 발견 3번은 이 게이트가 실제로 도는 것을 전제로 한 지적이다.
  - `FlushMapTravelData` 가 `FSoftObjectPath` 를 점(`.`) 없는 패키지 경로만으로 세우는 것(`:128`)은 유효하다 — `FTopLevelAssetPath::TrySetPath` 는 패키지 이름만 있는 문자열을 받아 `AssetName` 을 None 으로 둔다(`TopLevelAssetPath.cpp:586` 의 자체 테스트가 이를 보장). `GetAssetPath().GetPackageName()` 왕복 비교도 그래서 성립한다.
  - `CaptureActor`/`RestoreActor` 의 커스텀 버전 헤더 처리는 정상이다. `FArchiveProxy` 생성자가 내부 archive 상태를 복사한 뒤 프록시를 링크하므로, 리더에서 프록시 생성 **전에** `SetUEVer`/`SetCustomVersions` 를 부르는 현재 순서(`:414-420`)가 맞다.
  - `ShouldSave` 가 레벨 배치 인스턴스가 아니라 아키타입과 비교하는 것은, 값이 아키타입과 같아졌을 때 레코드를 지우는 분기(`:353-357`)와 합쳐지면 "레벨에서 오버라이드한 초기값으로 되돌아간다"는 이론적 구멍이 있다. 다만 현재 `UPROPERTY(SaveGame)` 은 `UWxDeviceStateTreeComponent::StateTag` 와 `AWxSpawner::bIsKilled` 둘뿐이고 어느 쪽도 인스턴스 편집이 열려 있지 않아 도달할 수 없으므로 발견으로 올리지 않았다(`WxDeviceStateTreeComponent.h:69` 의 `InitialState` TODO 가 열리면 그때 재검토 대상이다).
  - `WxStateTreeTask_SaveGame.h:42` 의 헤더 인라인 정의는 코딩 규칙 6 위반처럼 보이나, 같은 파일 13행에 예외 근거가 명시돼 있고 엔진 StateTree 태스크가 전부 같은 형태라 프로젝트 관례로 보아 발견으로 올리지 않았다.
  - `PlayerTransform` 의 Identity sentinel(별도 bool 없음)은 `WxSaveGame.h:84-91` 에 근거가 명시된 설계 결정이라 발견으로 올리지 않았다(원점 정확히 위에서 저장하면 "미설정"으로 읽히는 이론적 구멍은 남는다).
- **미검토 / 한계**: `IWxSavable` 구현체(`AWxSpawner`·`AWxDevice`)의 `OnSaveRestored` 멱등성은 각 모듈 리뷰 소관이라 이번 패스에서 깊게 보지 않았다(`HandleWorldInitializedActors` 와 `HandleLevelAddedToWorld` 가 같은 셀 액터에 이중으로 걸릴 수 있어, 그 멱등성이 실제 계약이다). 실제 세이브 파일 왕복·이기종 빌드 간 레코드 호환은 빌드/에디터 실행 없이 검증할 수 없어 정적 분석에 그쳤다. `UWxSaveLibrary` 를 호출하는 BP/WBP 측 사용 패턴(UI 슬롯 흐름)도 범위 밖이라, 발견 3·11번이 실제로 어떤 위젯에서 불리는지는 확인하지 않았다.

---
*문서 기준 커밋 `807a9da8` · 리뷰일 2026-08-24 · 소스 13파일 — `/module-review`로 갱신*
