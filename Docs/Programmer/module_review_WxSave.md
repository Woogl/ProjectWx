# WxSave — 코드 리뷰

> 슬롯 수명·직렬화 버전 헤더·트래블 가드까지 위험 지점을 의식하고 설계한 흔적이 뚜렷하고, 널 체크와 계약 주석이 고르게 갖춰진 모듈이다. 남은 문제는 대부분 "실패 경로에서 무엇을 보존하고 무엇을 알릴 것인가" 에 몰려 있다. 이번 리뷰는 13개 소스 전부를 열어 두 서브시스템의 cpp(직렬화·플러시·복원·트래블)와 스폰 컴포넌트·ST 태스크를 라인 단위로 보고, 엔진 측 계약(`FStateTreeTaskCommonBase` 틱 기본값, `UGameplayStatics::AsyncSaveGameToSlot` 의 동기 직렬화)까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 6 |
| 🟢 사소 | 5 |

## 결과

### 1. 🔴 `LoadFromFile` 이 "파일 없음" 과 "파일 손상" 을 구분하지 않아, 읽기 실패한 세이브를 다음 오토세이브가 덮어써 영구 손실시킨다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:71`, `:79-85` (특히 `:83`)
- **범주**: 버그/정확성 (데이터 손실)
- **문제**: `UGameplayStatics::LoadGameFromSlot` 은 파일 부재뿐 아니라 **헤더 불일치·바이트 손상·SaveGame 클래스 미해석** 에서도 동일하게 nullptr 을 반환한다. 코드는 이 둘을 한 갈래로 묶어 같은 슬롯 이름으로 빈 SaveGame 을 만들고(`:83`) 트래블을 이어간다. 활성 슬롯 이름이 그대로 유지되므로, 이후 첫 체크포인트 오토세이브(`SaveToFile(FString(), ...)`)가 **아직 디스크에 남아 있던 원본 파일을 빈 세이브로 덮어쓴다**. 구체 시나리오: 비동기 디스크 기록(`:282-309`) 도중 크래시/전원 차단으로 파일이 잘림 → 다음 실행에서 로드 실패 → 빈 슬롯으로 리셋 → 체크포인트 한 번으로 플레이어 진행이 전부 소멸. 사망 부활 경로가 매번 `LoadFromFile("")` 로 활성 슬롯을 다시 읽으므로 트리거 빈도도 높다. 진단조차 `Log` 레벨 한 줄(`:84`)이라 표면화되지 않는다. 「파일 없어도 리셋 후 트래블」 은 의도된 결정이지만(주석 `:81-82`), 그 의도가 **파일이 존재하는데 못 읽는 경우까지** 포함하도록 설계된 흔적은 없다.
- **제안**: 이미 있는 `DoesSaveFileExist`(`:170`)로 두 경우를 가른다. 파일이 존재하는데 로드에 실패했다면 (a) 손상 파일을 `<Slot>.corrupt` 로 백업하거나 활성 슬롯 이름을 임시 슬롯으로 돌려 덮어쓰기를 차단하고, (b) `Error` 로그 + 호출자에게 실패를 알린다. 트래블(월드 리로드) 자체는 지금처럼 이어가도 사망 부활 의미론은 유지된다.
- **확신도**: 중간 (리셋+트래블 폴백 자체는 의도된 설계지만, 손상 파일 덮어쓰기 파급까지 의도된 것으로 보이진 않음)

### 2. 🟡 `FlushPlayerStats` 는 캡처 실패 시 기존 저장 스탯을 지워버린다 — 같은 파일의 `FlushPlayerTransform` 과 비대칭
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:196-198` (대조: `:163-169`)
- **범주**: 버그/정확성 (데이터 손실)
- **문제**: `SaveGame->PlayerStats.Reset()` 을 먼저 하고 `CapturePlayerStats` 를 호출한 뒤 `bHasPlayerStats = PlayerStats.Num() > 0` 로 확정한다. 폰은 있는데 캡처가 0건이면(ASC 없는 폰을 빙의 중 — 탈것·연출용 폰·스펙테이터, 또는 `GetSpawnedAttributes()` 가 아직 비어 있는 초기화 창) 이전에 저장돼 있던 스탯이 통째로 사라지고 `bHasPlayerStats=false` 가 되어, 로드 후 데이터테이블 기본 스탯으로 되돌아간다. 이 플러시는 곧바로 디스크 기록으로 이어지므로 메모리뿐 아니라 파일까지 손실된다. 바로 위 `FlushPlayerTransform` 은 같은 상황(폰 부재)에서 "이전 캡처 보존" 으로 명시 처리하는데(`:166-169`) 스탯 경로만 방어가 없다.
- **제안**: 로컬 `TMap` 에 캡처한 뒤 `Num() > 0` 일 때만 `PlayerStats` 를 교체하고 `bHasPlayerStats` 를 갱신한다(실패 시 기존 값 보존 + `Warning` 로그).
- **확신도**: 중간 (현재 플레이어 폰은 ASC 를 직접 소유해 재현 조건이 좁지만, 방어 비대칭은 코드상 명확)

### 3. 🟡 `PlayerTransform` 과 `TravelData.Map` 이 원자적으로 갱신되지 않아 맵 게이트가 오탐할 수 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:119` · `:166-169` · `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:210-215`
- **범주**: 버그/정확성
- **문제**: `TryGetPlayerTransform` 은 "저장 맵 == 현재 맵" 게이트로 크로스맵 좌표 오적용을 막는데(`WxSaveGameSubsystem.cpp:210-215`), 이 게이트가 성립하려면 `PlayerTransform` 과 `TravelData.Map` 이 한 저장에서 같이 쓰여야 한다. 그런데 `FlushMapTravelData` 는 현재 맵을 **무조건** 다시 스탬프하고(`:119`), `FlushPlayerTransform` 은 `ResumeTransform` 도 폰도 없으면 **이전 캡처를 보존한 채 조기 반환**한다(`:166-169`). 실패 시나리오: 맵 A 체크포인트 저장 → 같은 세션에서 맵 B 로 이동(teardown 은 트래블 데이터를 건드리지 않음) → 맵 B 에서 폰이 없는 순간(사망 직후·빙의 전) 명시 저장 → 파일에 `Map=B` + `PlayerTransform=A의 좌표` 가 남는다. 이후 로드하면 게이트를 통과해 맵 B 에서 맵 A 좌표로 스폰한다(지오메트리 내부/월드 밖 가능).
- **제안**: 폰 부재로 캡처하지 못했는데 저장 맵이 바뀌었다면 `PlayerTransform` 을 Identity(미설정 sentinel)로 리셋하거나, 재개 지점을 `{Map, Transform}` 한 구조체로 묶어 항상 같이 쓴다.
- **확신도**: 중간 (트리거 조건이 좁음 — 현재 저장 경로는 대부분 폰이 살아 있음)

### 4. 🟡 디스크 기록 실패가 로그로만 끝나고 호출자·UI 로 전달되는 경로가 없다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:294-308` · `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h:62` · `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h:38`
- **범주**: 설계/구조 (미처리 실패 경로)
- **문제**: `AsyncSaveGameToSlot` 의 완료 델리게이트는 성공/실패를 `Log`/`Warning` 으로 찍기만 한다. `SaveToFile` 은 `void` 이고 BP 래퍼도 `void` 라, UI 는 디스크 풀·권한·클라우드 동기화 잠금으로 기록이 실패해도 "저장됨" 으로 진행한다. 게다가 명명 저장 경로는 기록 **전에** 활성 슬롯 정체성을 새 이름으로 바꿔두므로(`:138-142`), 실패한 슬롯을 이후 체크포인트 오토세이브가 계속 목표로 삼아 실패가 누적된다. 세이브 시스템에서 "쓰기 실패를 아무도 모른다" 는 것은 기능 공백에 가깝다.
- **제안**: 서브시스템에 저장 완료 멀티캐스트(슬롯명 + 성공 여부)를 노출하거나 `SaveToFile` 에 완료 콜백 인자를 추가해 UI 가 실패 토스트/재시도를 띄우게 한다. 실패 로그는 `Error` 로 승격.
- **확신도**: 높음 (코드 사실). 우선순위는 UI 요구사항에 따라 판단

### 5. 🟡 `ResumeTransform` 원시 포인터가 헤더에 명시된 "비동기 확장 seam" 과 정면 충돌한다
- **위치**: `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h:40` (대조: `:31`) · `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:43-47`
- **범주**: 설계/구조 (객체 수명주기)
- **문제**: `RequestSaveFlush(..., const FTransform* ResumeTransform)` 는 호출자 스택의 임시 객체를 원시 포인터로 받고, ST 태스크는 지역 변수 `ResumeTransform` 의 주소를 넘긴다(`WxStateTreeTask_SaveGame.cpp:44-47`). 현재는 플러시가 전부 동기라 안전하다. 그러나 같은 헤더 `:31` 이 이 델리게이트 시그니처를 "비동기 작업이 생길 때 지연 완료로 되돌릴 seam" 이라고 명시한다 — 그 확장을 실제로 하는 순간 `FlushPlayerTransform` 이 이미 파괴된 스택 객체를 역참조하고, 증상은 "재개 지점이 가끔 쓰레기 좌표" 라는 재현 어려운 형태로 나타난다. 확장 지점을 문서화해 두고 그 확장이 곧바로 깨뜨릴 API 를 남겨둔 셈이다.
- **제안**: `const FTransform*` 를 `TOptional<FTransform>` 값 전달로 바꾼다(`SaveToFile` 시그니처 `WxSaveGameSubsystem.h:62` 포함). 호출부는 3곳뿐이고 의미론은 그대로다.
- **확신도**: 높음 (현재 동작은 정상 — 명시된 확장 지점과의 충돌이 근거)

### 6. 🟡 `bSaveInProgress` 가 저장 요청 단위가 아닌 서브시스템 전역 단일 플래그다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:145`, `:294-299` · `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:64`
- **범주**: 설계/구조 (상태 관리)
- **문제**: `SaveToFile` 이 플래그를 세우고(`:145`) 비동기 기록 콜백이 내리는데(`:298`), 저장 요청을 구분하지 않는다. 저장 A 진행 중 저장 B 가 시작되면 A 의 콜백이 B 의 기록 도중 플래그를 내리고, `IsSaveInProgress()` 를 폴링하는 ST 태스크(`WxStateTreeTask_SaveGame.cpp:64`)는 자기 저장이 끝나기 전에 Succeeded 로 빠진다. 반대로 무관한 다른 저장이 진행 중이면 이미 끝난 태스크가 계속 Running 으로 머문다. 데이터 자체는 `AsyncSaveGameToSlot` 이 게임 스레드에서 동기 직렬화하므로 손상되지 않지만(엔진 확인 완료), "저장 완료를 기다린다" 는 태스크 계약은 깨진다.
- **제안**: 진행 중 요청 수 카운터(`int32 PendingSaveCount`)로 바꾸면 조기 완료는 막힌다. 태스크별 정확성이 필요하면 요청 ID 를 발급해 `IsSaveInProgress(RequestId)` 로 폴링한다.
- **확신도**: 높음 (동시 저장이 실제로 겹치는 빈도는 낮음)

### 7. 🟡 `OnPossessedPawnChanged` 구독이 `OnUnregister` 에서 해제되지 않는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:56` (대조: `:39-45`)
- **범주**: 버그/정확성 (객체 수명주기)
- **문제**: `OnRegister` 가 건 `PostLoginHandle` 은 `OnUnregister` 가 정확히 해제하지만(`:41-42`), `HandleGameModePostLogin` 이 오너 PC 에 건 `OnPossessedPawnChanged` 동적 구독은 어디서도 `RemoveDynamic` 되지 않는다. 이 컴포넌트는 Experience 의 컴포넌트 주입 액션으로 붙고 떨어지는 대상이라(헤더 `WxPlayerSpawnComponent.h:21`), Experience 전환·GameFeature 비활성으로 컴포넌트만 언레지스터되고 PC 는 살아 있는 경우 구독이 남는다. 그 상태에서 빙의가 일어나면 이미 떨어진 컴포넌트가 `ApplySavedPlayerStats` 를 실행해, 의도치 않게 저장 스탯이 현재 폰에 덮인다.
- **제안**: `OnUnregister` 에서 `GetController<APlayerController>()` 를 얻어 `RemoveDynamic(this, &UWxPlayerSpawnComponent::HandlePossessedPawnChanged)` 를 호출한다(구독/해제 대칭).
- **확신도**: 중간 (GC 된 오브젝트는 동적 델리게이트가 스스로 정리하므로, 실제 문제는 "살아 있지만 언레지스터된" 경우로 한정)

### 8. 🟢 트래블 가드 `bTravelingFromSaveFile` 가 단일 래치라 해제 실패 시 세션 내내 자동 캡처가 조용히 멈춘다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:116`, `:246` · `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:83-86`
- **범주**: 설계/구조 (상태 관리)
- **문제**: 가드를 내리는 경로는 `ServerTravel` 즉시 실패(`:123`)와 목적지 월드의 `OnWorldBeginPlay` 보고(`WxSaveWorldSubsystem.cpp:83-86`) 둘뿐이다. `ServerTravel` 이 `true` 를 반환한 뒤 목적지 월드가 `UWxSaveWorldSubsystem` 을 만들지 않으면(`ShouldCreateSubsystem` 이 거르는 클라이언트/비게임 월드로 폴백) 가드가 켜진 채 남고, `HandleLevelRemovedFromWorld`·`HandleWorldBeginTearDown` 의 자동 캡처가 GameInstance 수명 내내 전부 스킵된다. 스킵 로그는 `Verbose` 라 아무도 눈치채지 못한다.
- **제안**: 트래블 시작 후 일정 프레임 내 완료 보고가 없으면 가드를 자동 해제하는 워치독을 두거나, 최소한 장기 유지 시 `Warning` 을 남긴다. `LoadFromFile` 진입 시 가드를 재설정해 스턱이 다음 로드로 해소되게 하는 것도 방법이다.
- **확신도**: 낮음 (정상 스탠드얼론 경로에선 재현되지 않으며 의도된 단순화일 수 있음 — 견고성 관점)

### 9. 🟢 플레이어 스탯 키가 어트리뷰트 프로퍼티 이름뿐이라 AttributeSet 이 둘 이상이 되면 조용히 섞인다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:228`, `:261`
- **범주**: 버그/정확성 (잠재)
- **문제**: 캡처는 `OutStats.Add(It->GetFName(), ...)`, 적용은 `InStats.Find(It->GetFName())` 로 **AttributeSet 클래스를 무시한 평면 이름 키**를 쓴다. 이 순회는 `GetSpawnedAttributes()` 전체를 돌도록 일부러 일반화돼 있으므로, 같은 이름의 어트리뷰트를 가진 두 번째 AttributeSet 이 추가되는 순간 캡처는 뒤에 온 값이 앞 값을 덮고 적용은 한 값이 양쪽에 뿌려진다. 현재는 `UWxCombatAttributeSet` 하나뿐이라 발현하지 않는다.
- **제안**: 키를 `<AttributeSet 클래스명>.<프로퍼티명>` 으로 바꾼다. 기존 슬롯과 호환이 깨지므로 적용 시 구 평면 키 폴백을 한 버전 남기는 편이 안전하다.
- **확신도**: 중간 (현 시점 미발현, 세트 추가 시 확정적으로 발현)

### 10. 🟢 규칙 위반 — 델리게이트 콜백 `Handle` prefix 누락 + 불필요한 람다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:160`, `:292-309`
- **범주**: 규칙 위반 (CLAUDE.md 코딩 규칙 4 — 델리게이트 콜백 `Handle` prefix, 규칙 3 — 불필요한 람다)
- **문제**: (1) `ContinueSaveToFileToDisk` 는 `FOnSaveFlushComplete::FDelegate::CreateUObject` 로 바인딩되는 콜백인데 `Handle` prefix 가 없다(`:160`). 직접 호출(`:166`)을 겸하는 continuation 이라 순수 핸들러와 결이 다르긴 하다. (2) `AsyncSaveGameToSlot` 완료 델리게이트(`:292-309`)는 `TWeakObjectPtr` 캡처 람다인데, `FAsyncSaveGameToSlotDelegate::CreateUObject(this, &...)` 로 멤버를 바인딩하면 수명 처리가 자동이라 람다가 필요 없다. 반면 `Wx.Save.Dump` 의 람다(`:16`)는 정적 초기화 시점이라 멤버 바인딩이 불가능해 정당하다. 그 외 콜백(`HandleWorldInitializedActors`·`HandleGameModePostLogin`·`HandlePossessedPawnChanged`)은 규칙을 지킨다.
- **제안**: `:292` 람다를 `HandleSaveGameToSlotComplete(const FString&, int32, bool)` 멤버로 빼고 `CreateUObject` 로 바인딩한다(항목 4 의 실패 통지도 같은 자리에 얹으면 된다). `ContinueSaveToFileToDisk` 는 델리게이트 전용 얇은 `HandleSaveFlushComplete` 를 두고 그 안에서 호출하는 형태가 규칙과 의미를 모두 지킨다.
- **확신도**: 높음

### 11. 🟢 서브시스템/SaveGame 획득 체인이 4개 함수에 그대로 복제돼 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:108-111`, `:125-129`, `:147-151`, `:179-183`
- **범주**: 중복/복잡도
- **문제**: `GetWorld() → GetGameInstance() → GetSubsystem<UWxSaveGameSubsystem>() → GetSaveGame()` 4단 널 체크가 `FlushMapTravelData`·`FlushSavableActors`·`FlushPlayerTransform`·`FlushPlayerStats` 에 반복된다. 실패 처리도 제각각이라(둘은 `Warning`, 둘은 무로그) 일관성이 없고, 항목 2·3 같은 "조용한 조기 반환" 이 눈에 띄지 않는 원인이기도 하다.
- **제안**: private 헬퍼 `UWxSaveGame* GetActiveSaveGame() const` 하나로 접고, 로그는 필요한 호출부에만 남긴다.
- **확신도**: 높음

### 12. 🟢 존재 확인을 위한 `for (...) { return; }` 루프는 버그처럼 읽힌다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:60-63`
- **범주**: 중복/복잡도
- **문제**: `APlayerStartPIE` 존재 여부만 보면 되는데 `TActorIterator` 루프 본문이 무조건 `return` 이다. 동작은 의도대로지만(하나라도 있으면 조기 반환) 리뷰어에게는 "첫 바퀴에서 무조건 나가는 실수" 로 보이고, 재개 지점 주입 로직 전체가 이 한 줄에 매달려 있어 잘못 건드리기 쉽다.
- **제안**: `const bool bHasPIEStart = (bool)TActorIterator<APlayerStartPIE>(GetWorld());` 처럼 의도를 이름으로 드러낸다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`(직렬화·버전 헤더·자동 캡처/복원·ASC 스탯 2패스 적용), `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`(슬롯 수명·트래블 가드·비동기 디스크 기록), `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`(스폰 경로 주입·스탯 복원 타이밍), `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`(데이터 모델·버전 헤더 설계), `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/WxSave.uplugin`, `Plugins/WxSave/README.md`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`
- **확인했으나 발견 없음**: `WxSave.Build.cs:11-21` 과 `WxSave.uplugin` 은 `WxCore` + 엔진 모듈만 의존 — 「WxCore 외 Wx 플러그인 참조 금지」 준수 ✅. `BlueprintCallable` 은 `UWxSaveLibrary`(BlueprintFunctionLibrary)에만 ✅. 전 소스 첫 줄 Copyright ✅, `Wx` prefix ✅, UObject override 의 `Super::` 호출 ✅. `FWxStateTreeTask_SaveGame` 이 생성자에서 `bShouldCallTick` 을 세우지 않아도 UE 5.8 `FStateTreeTaskCommonBase` 는 기본값 `true` 라 폴링이 동작한다(엔진 `StateTreeTaskBase.h:25,165` 대조). `AsyncSaveGameToSlot` 은 게임 스레드에서 동기 직렬화 후 파일 쓰기만 비동기라, 기록 중 SaveGame 오브젝트가 변경돼도 데이터 레이스는 없다(엔진 `GameplayStatics.cpp:2403-2426` 대조). `CaptureActor`/`RestoreActor` 의 버전 헤더 왕복과 `TMap::FindOrAdd` 참조 수명, 월드 필터링(`Params.World != GetWorld()`)도 정확하다. 헤더의 1줄 getter 인라인 정의는 프로젝트 전 플러그인 공통 관행이라 지적에서 제외했다.
- **미검토 / 한계**: 실제 세이브 파일 왕복(저장→종료→로드) 동작 검증은 하지 않았다. `IWxSavable`·`GetSaveId()` 의 GUID 부여 경로는 `WxCore`/`WxWorld` 소유라 범위 밖(중복 GUID 액터가 있으면 레코드가 충돌하지만 방지 책임은 이 모듈 밖이다). `ST_CheckPoint` 의 태스크 실행 순서(회복 GE 와 `Save Game` 중 무엇이 먼저인지 — 저장되는 HP 값을 좌우한다)는 ST 에셋 영역이라 보지 않았고, 사망 화면·명명 슬롯 UI 등 `UWxSaveLibrary` 를 부르는 BP/WBP 호출부도 범위 밖이다. 「세이브 슬롯 월드 적용은 명시 로드 시점에만」 은 의도된 설계로 보고 지적하지 않았다. `FObjectAndNameAsStringProxyArchive` 를 `bLoadIfFindFails=false` 로 쓰는 선택은 미로드 에셋 참조를 null 로 만들 수 있으나, 현재 프로젝트의 `UPROPERTY(SaveGame)` 이 `FGameplayTag` 등 POD 뿐이라 발현 여지가 없어 발견으로 올리지 않았다.

---
*문서 기준 커밋 `c37b6fa6` · 리뷰일 2026-07-31 · 소스 13파일 — `/module-review`로 갱신*
