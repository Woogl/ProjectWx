# WxSave — 코드 리뷰

> 직렬화 버전 헤더·트래블 가드·GUID 키잉 같은 세이브 시스템의 고전적 함정을 미리 알고 짠 흔적이 뚜렷하고 널 체크와 계약 주석의 밀도가 높은 모듈이며, 남은 문제는 대부분 "실패 경로에서 무엇을 보존하고 무엇을 알릴 것인가" 한 축에 몰려 있다. 이번 리뷰는 13개 소스를 모두 열되 두 서브시스템 cpp(직렬화·플러시·복원·트래블)·스폰 컴포넌트·ST 태스크를 라인 단위로 읽었고, 판정이 엔진 계약에 걸리는 지점(`AsyncSaveGameToSlot`/`LoadGameFromMemory` 의 실패 반환, `UWorld::ServerTravel` 의 클라 동작, `FStateTreeTaskBase` 의 틱 기본값, `FTopLevelAssetPath` 의 패키지-only 경로, 동적 멀티캐스트 `Add` 의 중복 허용)은 5.8 엔진 소스와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 6 |
| 🟢 사소 | 5 |

## 결과

### 1. 🔴 `LoadFromFile` 이 "파일 없음" 과 "파일 못 읽음" 을 한 갈래로 묶어, 읽지 못한 세이브를 다음 오토세이브가 덮어쓴다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:79`, `:87-92`(특히 `:90`)
- **범주**: 버그/정확성 (데이터 손실)
- **문제**: `UGameplayStatics::LoadGameFromSlot` 은 파일 부재뿐 아니라 빈/손상 버퍼, SaveGame 클래스 미해석(`GameplayStatics.cpp:2459-2486` — 클래스 이름을 못 찾으면 nullptr)에서도 똑같이 nullptr 을 반환하고, `Cast<UWxSaveGame>` 실패도 같은 갈래로 흡수된다. 코드는 이 둘을 구분하지 않고 **같은 슬롯 이름으로** 빈 SaveGame 을 만든 뒤(`:90`) 트래블을 이어간다. 활성 슬롯 정체성이 그대로 유지되므로 이후 첫 체크포인트 오토세이브(`SaveToFile(FString(), ...)`)가 아직 디스크에 남아 있던 원본 파일을 빈 세이브로 덮어써 진행이 영구 소멸한다. 구체 시나리오: (a) 비동기 기록 중 크래시로 파일이 잘림, (b) SaveGame 서브클래스(BP)를 리네임·삭제한 빌드로 기존 파일을 여는 경우 — 파일은 멀쩡한데 클래스 해석에 실패해 같은 결과가 된다. 사망 부활이 매번 `LoadFromFile("")` 로 활성 슬롯을 다시 읽는 구조라 트리거 빈도도 낮지 않고, 진단은 `Log` 한 줄(`:91`)뿐이라 표면화되지 않는다. 「파일이 없어도 리셋 후 트래블」은 주석대로 의도된 결정이지만, 그 의도가 **파일이 존재하는데 못 읽는 경우**까지 포함하도록 설계된 흔적은 없다.
- **제안**: 이미 있는 `DoesSaveFileExist`(`:173`)로 두 경우를 가른다. 파일이 있는데 로드에 실패했으면 (a) 손상 파일을 `<Slot>.corrupt` 로 옮기거나 활성 슬롯을 임시 이름으로 돌려 덮어쓰기를 차단하고, (b) `Error` 로그와 함께 호출자에게 실패를 알린다. 트래블(월드 리로드) 자체는 지금처럼 이어가도 사망 부활 의미론은 유지된다.
- **확신도**: 중간 (리셋+트래블 폴백은 의도된 설계지만, 손상·미해석 파일 덮어쓰기 파급까지 의도된 것으로 보이지 않음)

### 2. 🟡 `FlushPlayerStats` 는 캡처가 0건이면 기존 저장 스탯을 지운다 — 바로 위 `FlushPlayerTransform` 과 방어가 비대칭이다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:194-196` (대조: `:161-167`)
- **범주**: 버그/정확성 (데이터 손실)
- **문제**: `PlayerStats.Reset()` 을 먼저 하고 `CapturePlayerStats` 를 부른 뒤 `bHasPlayerStats = Num() > 0` 으로 확정한다. 폰은 있는데 캡처가 0건이면(ASC 없는 폰을 빙의 중 — 연출 폰·스펙테이터, 또는 `GetSpawnedAttributes()` 가 아직 비어 있는 초기화 창) 이전에 저장돼 있던 스탯이 통째로 사라지고 `bHasPlayerStats=false` 가 되어 로드 후 데이터테이블 기본값으로 되돌아간다. 이 플러시는 곧장 디스크 기록으로 이어지므로 메모리뿐 아니라 파일까지 잃는다. 같은 파일의 `FlushPlayerTransform` 은 동일 상황(캡처 불가)을 "이전 캡처 보존" 으로 명시 처리하는데(`:164-167`) 스탯 경로만 그 방어가 없다.
- **제안**: 지역 `TMap` 에 캡처한 뒤 `Num() > 0` 일 때만 `SaveGame->PlayerStats` 를 교체하고 `bHasPlayerStats` 를 갱신한다(실패 시 기존 값 보존 + `Warning`).
- **확신도**: 중간 (현재 플레이어 폰이 ASC 를 직접 소유해 재현 조건이 좁지만, 방어 비대칭 자체는 코드상 명확)

### 3. 🟡 디스크 기록 실패가 로그로만 끝나고 호출자·UI 로 전달되는 경로가 없다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:293-309`(특히 `:301-308`) · `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h:62` · `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h:38`
- **범주**: 설계/구조 (미처리 실패 경로)
- **문제**: `AsyncSaveGameToSlot` 완료 델리게이트가 성공/실패를 `Log`/`Warning` 으로 찍기만 한다. `SaveToFile` 도 BP 래퍼도 `void` 라, 디스크 풀·권한·클라우드 동기화 잠금으로 기록이 실패해도 UI 는 "저장됨" 으로 진행한다. 엔진 구현상 슬롯명이 비었거나 직렬화가 실패하면 델리게이트가 `bSuccess=false` 로 **동기 발화**하고(`GameplayStatics.cpp:2409-2425`) 비동기 완료도 게임 스레드에서 발화하므로 실패는 확실히·안전하게 감지되는데, 그 정보를 아무도 받지 않는 것이다. 게다가 명명 저장 경로는 기록 **전에** 활성 슬롯 정체성을 새 이름으로 바꾸므로(`:143-147`), 한 번 실패한 슬롯을 이후 체크포인트 오토세이브가 계속 목표로 삼아 실패가 누적된다. 세이브 시스템에서 "쓰기 실패를 아무도 모른다" 는 것은 사실상 기능 공백이다.
- **제안**: 서브시스템에 저장 완료 멀티캐스트(슬롯명 + 성공 여부)를 노출하거나 `SaveToFile` 에 완료 콜백 인자를 추가해 UI 가 실패를 표면화하게 한다. 실패 로그는 `Error` 로 승격.
- **확신도**: 높음 (코드 사실. 우선순위는 UI 요구사항에 따라 판단)

### 4. 🟡 BP 진입점에 권위 게이트가 없어, 클라에서 부르면 스테일 저장·세션 이탈이 나고 가드 해제점이 사라진다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:161-170`(`:169`), `:122` · `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:51`, `:82-85` · `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp:58-82`
- **범주**: 설계/구조 (권위 모델)
- **문제**: `SaveToFile` 은 월드 서브시스템을 못 찾으면 "플러시할 것이 없으니" 바로 기록한다(`:169`). 그러나 서브시스템 부재는 "플러시할 것이 없다" 와 동치가 아니다. `ShouldCreateSubsystem` 이 **클라이언트 월드를 통째로 배제**하므로(`WxSaveWorldSubsystem.cpp:51`), 클라에서 BP 가 `UWxSaveLibrary::SaveToFile` 을 부르면 라이브 상태가 하나도 반영되지 않은 인메모리 SaveGame 이 클라 디스크에 정상 저장처럼 기록된다. 로드 경로는 더 나쁘다: `UWorld::ServerTravel` 은 `GetAuthGameMode()` 가 null 인 클라에서도 그대로 `NextURL` 을 세우고 `NextSwitchCountdown=0` 으로 즉시 전환하므로(엔진 `World.cpp:9525-9558`), 클라 UI 가 `LoadFromFile`/`TravelFromSaveFile` 을 부르면 그 클라만 세션에서 이탈해 로컬 맵을 연다. 같은 맥락에서 `bTravelingFromSaveFile`(`:122`)의 해제점이 **새 월드의 비-클라 월드 서브시스템 `OnWorldBeginPlay` 단 하나뿐**이라는 점도 취약하다 — 트래블이 시작된 뒤 목적지 맵 로드가 실패하거나 새 월드가 클라 넷모드로 뜨면 가드가 세션 내내 남아 스트리밍-아웃·맵 이탈 자동 캡처가 전부 조용히 꺼진다. README 는 "저장은 서버 소유, 클라 진입은 노옵" 을 계약으로 적어 두었지만, 그 게이트가 실제로 있는 곳은 ST 태스크(`WxStateTreeTask_SaveGame.cpp:26-30`)뿐이고 BP 진입점에는 없다.
- **제안**: 진입점(서브시스템 또는 라이브러리)에서 권위를 먼저 판정해 클라 호출을 명시적 노옵 + `Warning` 으로 돌린다. 가드는 트래블 시작 프레임/월드 카운터 기준의 만료 조건을 하나 더 둬 해제 실패가 영구화되지 않게 한다.
- **확신도**: 중간 (현재 스탠드얼론 단일 플레이 전제라 미발현. 멀티 진입 시 확정적으로 발현)

### 5. 🟡 `PlayerTransform` 과 `TravelData.Map` 이 원자적으로 갱신되지 않아 맵 게이트가 오탐할 수 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:117` · `:161-169` · `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:203-220`(게이트 `:211-216`)
- **범주**: 버그/정확성
- **문제**: `TryGetPlayerTransform` 의 "저장 맵 == 현재 맵" 게이트는 두 필드가 한 저장에서 같이 쓰였다는 전제 위에 서 있다. 그런데 `RequestSaveFlush` 는 `FlushMapTravelData` → `FlushPlayerTransform` 순으로 부르면서(`:33-34`), 앞의 것은 현재 맵을 **무조건** 다시 스탬프하고(`:117`) 뒤의 것은 `ResumeTransform` 도 폰도 없으면 이전 값을 남긴 채 조기 반환한다(`:164-167`). 실패 시나리오: 맵 A 에서 체크포인트 저장 → 같은 세션에서 맵 B 로 이동(teardown 은 트래블 데이터를 건드리지 않는다 — `:504-529`) → 맵 B 에서 폰이 없는 순간(사망 직후·빙의 전) 명시 저장 → 파일에 `Map=B` + `PlayerTransform=A 좌표` 가 남는다. 이후 로드하면 게이트를 통과해 맵 B 에서 A 좌표로 스폰한다(지오메트리 내부·월드 밖 가능).
- **제안**: 폰 부재로 캡처하지 못했는데 저장 맵이 바뀌었다면 `PlayerTransform` 을 Identity(미설정 sentinel)로 되돌리거나, 재개 지점을 `{Map, Transform}` 한 구조체로 묶어 항상 함께 쓴다.
- **확신도**: 중간 (트리거 조건이 좁다 — 현재 저장 경로는 대부분 폰이 살아 있다)

### 6. 🟡 `bSaveInProgress` 가 저장 요청 단위가 아닌 서브시스템 전역 단일 플래그다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:150`, `:296-299` · `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:61`
- **범주**: 설계/구조 (상태 관리)
- **문제**: `SaveToFile` 이 플래그를 세우고(`:150`) 비동기 기록 콜백이 내리는데(`:298`), 요청을 구분하지 않는다. 저장 A 진행 중 저장 B 가 시작되면 A 의 콜백이 B 의 기록 도중 플래그를 내리고, `IsSaveInProgress()` 를 폴링하는 ST 태스크는 자기 저장이 끝나기 전에 Succeeded 로 빠진다. 반대로 무관한 저장이 진행 중이면 이미 끝난 태스크가 계속 Running 으로 남는다. 데이터 자체는 `AsyncSaveGameToSlot` 이 게임 스레드에서 `SaveGameToMemory` 를 동기 수행한 뒤 바이트 배열만 비동기로 쓰므로 손상되지 않지만, "저장 완료를 기다린다" 는 태스크 계약은 깨진다.
- **제안**: 진행 중 요청 카운터(`int32 PendingSaveCount`)로 바꾸면 조기 완료는 막힌다. 태스크별 정확성이 필요하면 요청 ID 를 발급해 `IsSaveInProgress(RequestId)` 로 폴링한다.
- **확신도**: 높음 (동시 저장이 실제로 겹치는 빈도는 낮음)

### 7. 🟡 스폰 컴포넌트의 구독/셋업이 비대칭·비멱등이다 — `OnPossessedPawnChanged` 미해제
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:56`, `:32-36`, `:77-87` (대조: `:39-45`)
- **범주**: 버그/정확성 (객체 수명주기)
- **문제**: `OnRegister` 가 건 `PostLoginHandle` 은 `OnUnregister` 가 정확히 해제하는데(`:41-42`), `HandleGameModePostLogin` 이 오너 PC 에 건 동적 구독(`:56`)은 어디서도 `RemoveDynamic` 되지 않는다. 이 컴포넌트는 Experience 의 컴포넌트 주입 액션으로 붙고 떨어지는 대상이므로(헤더 `WxPlayerSpawnComponent.h:21`), 컴포넌트만 언레지스터되고 PC 는 살아 있는 전환에서 구독이 남고, 그 상태로 빙의가 일어나면 이미 떨어진 컴포넌트가 `ApplySavedPlayerStats` 를 실행해 저장 스탯이 현재 폰에 덮인다. 같은 비대칭 탓에 재등록 시 중복도 생긴다 — `OnRegister` 의 캐치업 분기는 `PlayerState` 유무만 보므로(`:33`) 재등록 시 무조건 다시 통과하고, `HandleGameModePostLogin` 은 멱등하지 않아 `AddDynamic` 이 중복 바인딩되며(엔진 `ScriptDelegates.h:1544-1555` — `AddInternal` 은 중복을 막지 않고 `ensure` 만 때린다) 재개 지점 마커 `APlayerStart` 도 한 벌 더 스폰돼(`:80`) 레벨에 남는다.
- **제안**: `OnUnregister` 에서 `GetController<APlayerController>()` 로 `RemoveDynamic(this, &UWxPlayerSpawnComponent::HandlePossessedPawnChanged)` 를 호출해 대칭을 맞추고, `HandleGameModePostLogin` 진입에 1회 실행 플래그(또는 최소한 `AddUniqueDynamic` + 기존 마커 재사용)를 둔다.
- **확신도**: 중간 (GC 된 오브젝트는 동적 델리게이트가 스스로 정리하므로 실제 문제는 "살아 있지만 언레지스터된" 경우로 한정)

### 8. 🟢 규칙 위반 — 불필요한 람다 + 델리게이트 콜백의 `Handle` prefix 누락
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:294-309`, `:164`
- **범주**: 규칙 위반 (코딩 규칙 3 — 불필요한 람다, 규칙 4 — 콜백 `Handle` prefix)
- **문제**: (1) `AsyncSaveGameToSlot` 완료 델리게이트(`:294-309`)는 `TWeakObjectPtr` 를 캡처하는 람다인데, `FAsyncSaveGameToSlotDelegate::CreateUObject(this, &...)` 로 멤버를 바인딩하면 수명 처리가 자동이라 람다도 위크 포인터도 필요 없다. 반면 `Wx.Save.Dump` 의 람다(`:15-26`)는 정적 초기화 시점이라 멤버 바인딩이 불가능해 정당하다. (2) `ContinueSaveToFileToDisk` 는 `FOnSaveFlushComplete::FDelegate::CreateUObject` 로 바인딩되는 콜백인데(`:164`) `Handle` prefix 가 없다 — 직접 호출(`:169`)을 겸하는 continuation 이라 순수 핸들러와 결이 다르긴 하다. 나머지 콜백(`HandleWorldInitializedActors`·`HandleGameModePostLogin`·`HandlePossessedPawnChanged` 등)은 규칙을 지킨다.
- **제안**: `:294` 람다를 `HandleSaveGameToSlotComplete(const FString&, int32, bool)` 멤버로 빼고 `CreateUObject` 로 바인딩한다(항목 3 의 실패 통지도 같은 자리에 얹으면 된다). `ContinueSaveToFileToDisk` 는 델리게이트 전용 얇은 `HandleSaveFlushComplete` 를 두고 그 안에서 부르는 형태가 규칙과 의미를 모두 지킨다.
- **확신도**: 높음

### 9. 🟢 서브시스템/SaveGame 획득 체인이 8개 함수에 그대로 복제돼 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:106-108`, `:123-126`, `:145-148`, `:177-180`, `:425-427`, `:457-459`, `:481-483`, `:511-512`
- **범주**: 중복/복잡도
- **문제**: `GetWorld() → GetGameInstance() → GetSubsystem<UWxSaveGameSubsystem>() → GetSaveGame()` 4단 널 체크가 네 플러시 함수와 네 월드 이벤트 핸들러에 그대로 반복된다. 실패 처리도 제각각이라(둘은 `Warning`, 나머지는 무로그) 일관성이 없고, 항목 2·5 같은 "조용한 조기 반환" 이 눈에 띄지 않는 배경이기도 하다.
- **제안**: private 헬퍼(`UWxSaveGameSubsystem* GetGameSubsystem() const` / `UWxSaveGame* GetActiveSaveGame() const`)로 접고 로그는 필요한 호출부에만 남긴다.
- **확신도**: 높음

### 10. 🟢 플레이어 스탯 키가 어트리뷰트 프로퍼티 이름뿐이라 AttributeSet 이 둘 이상이 되면 조용히 섞인다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:226`, `:258`
- **범주**: 버그/정확성 (잠재)
- **문제**: 캡처는 `OutStats.Add(It->GetFName(), ...)`, 적용은 `InStats.Find(It->GetFName())` 로 **AttributeSet 클래스를 무시한 평면 이름 키**를 쓴다. 이 순회는 `GetSpawnedAttributes()` 전체를 도는 일반화된 구조이므로, 같은 이름의 어트리뷰트를 가진 두 번째 AttributeSet 이 추가되는 순간 캡처는 뒤에 온 값이 앞 값을 덮고 적용은 한 값이 양쪽에 뿌려진다. 현재는 세트가 하나라 발현하지 않는다.
- **제안**: 키를 `<AttributeSet 클래스명>.<프로퍼티명>` 으로 바꾼다. 기존 슬롯과 호환이 깨지므로 적용 측에 구 평면 키 폴백을 한 버전 남기는 편이 안전하다.
- **확신도**: 중간 (현 시점 미발현, 세트 추가 시 확정적으로 발현)

### 11. 🟢 `ResumeTransform` 원시 포인터가 헤더에 명시된 "비동기 확장 seam" 과 충돌한다
- **위치**: `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h:40`(대조: `:31`) · `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:41-44`
- **범주**: 설계/구조 (객체 수명주기)
- **문제**: `RequestSaveFlush(..., const FTransform* ResumeTransform)` 는 호출자 스택의 임시 객체를 원시 포인터로 받고, ST 태스크는 지역 변수의 주소를 넘긴다(`:41`, `:44`). 현재는 플러시가 전부 동기라 안전하다. 그러나 같은 헤더 `:31` 이 이 델리게이트를 "비동기 작업이 생길 때 지연 완료로 되돌릴 seam" 으로 명시해 두었고, 그 확장을 실제로 하는 순간 `FlushPlayerTransform` 이 파괴된 스택 객체를 역참조한다. 증상은 "재개 지점이 가끔 쓰레기 좌표" 라는 재현 어려운 형태로 나온다.
- **제안**: `const FTransform*` 를 `TOptional<FTransform>` 값 전달로 바꾼다(`WxSaveGameSubsystem.h:62` 포함). 호출부는 3곳뿐이고 의미론은 그대로다.
- **확신도**: 높음 (현재 동작은 정상 — 명시된 확장 지점과의 충돌이 근거)

### 12. 🟢 `RestoreActor` 가 모든 savable 액터의 Transform 을 무조건 되돌린다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:362` (캡처 측 `:294`)
- **범주**: 설계/구조
- **문제**: 레코드는 액터 종류와 무관하게 항상 Transform 을 담고(`:294`), 복원은 `SetActorTransform` 을 조건 없이 부른다(`:362`). 두 가지 부작용이 있다. (a) 레벨 디자이너가 세이브 대상 액터(스포너·기믹)를 에디터에서 옮겨도, 옛 슬롯을 들고 있는 플레이어에게는 이전 좌표로 되돌려진다 — 위치가 상태가 아닌 액터에게는 레벨 수정이 조용히 무효화되는 셈이다. (b) 루트가 Static mobility 인 액터(BP 기믹의 흔한 구성)에 대해서는 등록 이후 `SetActorTransform` 이 `CheckStaticMobilityAndWarn` 에 걸려 실패하고 경고만 남긴다. 현재 세이브 대상(`AWxSpawner`, `UWxGimmickStateTreeComponent` 호스트)은 어느 쪽도 위치가 상태가 아니라 실익 없이 위험만 있는 왕복이다.
- **제안**: Transform 왕복을 옵트인으로 돌린다 — `IWxSavable` 에 "위치도 저장 대상인가" 를 묻는 후크를 두거나, 루트 mobility 가 `Movable` 인 액터에만 적용한다.
- **확신도**: 낮음 (샘플 이식 골격을 그대로 유지한 의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`(직렬화·버전 헤더·자동 캡처/복원·ASC 스탯 2패스 적용), `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`(슬롯 수명·트래블 가드·비동기 디스크 기록), `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`(StartSpot 주입·스탯 복원 타이밍), `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`(데이터 모델·버전 헤더 설계), `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/WxSave.uplugin`, `Plugins/WxSave/README.md`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`(구현체·SaveId 부여 확인용), `Source/WxGame/Framework/WxGameMode.cpp`(스폰·RestartPlayer 순서 확인용)
- **확인했으나 발견 없음**: `WxSave.Build.cs:11-21` 과 `WxSave.uplugin` 은 `WxCore` + 엔진 모듈만 의존 — 「WxCore 외 Wx 플러그인 참조 금지」 준수. `BlueprintCallable` 은 `UWxSaveLibrary`(BlueprintFunctionLibrary)에만 사용. 전 소스 첫 줄 Copyright, `Wx` prefix, override 의 `Super::` 호출 모두 정상이며, 헤더 인라인 정의는 `GetInstanceDataType()` 하나뿐인데 프로젝트 전체 ST 노드의 공통 형태이고 헤더 `:13` 에 예외 사유가 명시돼 있다. 엔진 대조 결과도 코드 주장과 일치했다: `AsyncSaveGameToSlot` 은 `SaveGameToMemory` 를 게임 스레드에서 동기 수행하고 쓰기만 비동기로 넘기며 완료 콜백도 게임 스레드 발화(`GameplayStatics.cpp:2403-2426`)라 저장 중 SaveGame 변형에 의한 데이터 레이스는 없다. `FStateTreeTaskBase` 는 `bShouldCallTick(true)` 가 기본이라(`StateTreeTaskBase.h:25`) ST 태스크의 `Running` + 폴링 틱은 실제로 동작하고, `FWxStateTreeTask_SaveGame` 의 빈 생성자는 그 기본값에 기대는 무본문 생성자다. `FlushMapTravelData` 가 만드는 패키지-only `FSoftObjectPath`(`WxSaveWorldSubsystem.cpp:117`)는 엔진이 명시적으로 지원하는 형태라(`TopLevelAssetPath.cpp:146-153`) 왕복·맵 일치 비교가 성립한다. `FArchiveProxy` 가 커스텀 버전 컨테이너를 내부 archive 로 위임하므로(`ArchiveProxy.h:204-212`) `CaptureActor`/`RestoreActor` 의 버전 헤더 수집·적용도 유효하다. `TMap::FindOrAdd` 참조 수명, 월드 필터링(`Params.World != GetWorld()`), 트래블 가드의 스트리밍-아웃/teardown 스킵 조합, `Level->Actors` 의 널 원소 방어, 저장 대상의 `SaveId` 중복 방지(`WxSpawner.cpp:199-205` 의 `PostDuplicate` 재부여)도 정확하다.
- **미검토 / 한계**: 실제 세이브 파일 왕복(저장→종료→로드) 실행 검증은 하지 않았다(정적 리뷰). `ST_CheckPoint` 의 태스크 실행 순서(회복 GE 와 `Save Game` 중 무엇이 먼저인지 — 저장되는 HP 를 좌우한다)는 ST 에셋 영역이라 보지 않았고, `UWxSaveLibrary` 를 부르는 사망 화면·명명 슬롯 WBP 호출부도 범위 밖이다. `FObjectAndNameAsStringProxyArchive` 를 `bLoadIfFindFails=false` 로 쓰는 선택은 미로드 오브젝트 참조를 null 로 만들 수 있으나, 현재 프로젝트의 `UPROPERTY(SaveGame)` 은 `WxSpawner::bIsKilled`(bool)와 `WxGimmickStateTreeComponent::StateTag`(FGameplayTag) 둘뿐이라 발현 여지가 없어 발견으로 올리지 않았다. `CaptureActor` 가 SaveGame 프로퍼티가 없는 컴포넌트까지 빈 `ComponentData` 엔트리로 남기는 점, `FlushSavableActors`/`HandleWorldInitializedActors` 의 전 액터 순회(`TActorIterator` + 액터별 `FindComponentByInterface`, 후자는 액터당 `FindSavable` 2회 — `:439`, `:442`)는 오픈월드 액터 수가 커지면 파일 크기·저장 1회 비용에 영향을 줄 수 있으나 매 틱 경로가 아니라 성능 발견으로 세우지 않았다. `ActorRecords` 가 삭제된 액터의 레코드를 영구 보관하는 것(프루닝 없음)도 같은 이유로 제외했다. `WxSaveLibrary.cpp:10-33` 의 익명 namespace 헬퍼는 CLAUDE.md 규칙 대상이 아니라 발견에서 제외했다.

---
*문서 기준 커밋 `f7620119` · 리뷰일 2026-08-11 · 소스 13파일 — `/module-review`로 갱신*
