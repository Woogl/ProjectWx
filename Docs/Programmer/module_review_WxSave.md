# WxSave — 코드 리뷰

> 슬롯 수명·직렬화 버전 헤더·트래블 가드 등 세이브 시스템의 함정을 미리 알고 짠 흔적이 뚜렷하고, 널 체크와 계약 주석의 밀도가 높은 모듈이다. 남은 문제는 거의 전부 "실패 경로에서 무엇을 보존하고 무엇을 알릴 것인가" 한 축에 몰려 있다. 이번 리뷰는 13개 소스를 모두 열되 두 서브시스템 cpp(직렬화·플러시·복원·트래블), 스폰 컴포넌트, ST 태스크를 라인 단위로 읽었고, 판정이 엔진 계약에 걸리는 지점(`FStateTreeTaskCommonBase` 의 틱 기본값, `FMemoryReader` 의 커스텀 버전 리셋)은 5.8 엔진 소스와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 6 |
| 🟢 사소 | 5 |

## 결과

### 1. 🔴 `LoadFromFile` 이 "파일 없음" 과 "파일 손상" 을 한 갈래로 묶어, 읽지 못한 세이브를 다음 오토세이브가 덮어쓴다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:71`, `:79-85`(특히 `:83`)
- **범주**: 버그/정확성 (데이터 손실)
- **문제**: `UGameplayStatics::LoadGameFromSlot` 은 파일 부재뿐 아니라 바이트 손상·헤더 불일치·SaveGame 클래스 미해석에서도 똑같이 nullptr 을 반환한다. 코드는 그 둘을 구분하지 않고 **같은 슬롯 이름으로** 빈 SaveGame 을 만든 뒤(`:83`) 트래블을 이어간다. 활성 슬롯 정체성이 그대로 유지되므로, 이후 첫 체크포인트 오토세이브(`SaveToFile(FString(), ...)`)가 아직 디스크에 남아 있던 원본 파일을 빈 세이브로 덮어써 진행이 영구 소멸한다. 구체 시나리오: 비동기 디스크 기록(`:282-309`) 중 크래시·전원 차단으로 파일이 잘림 → 다음 실행에서 로드 실패 → 빈 슬롯 리셋 → 체크포인트 한 번으로 끝. 사망 부활이 매번 `LoadFromFile("")` 로 활성 슬롯을 다시 읽는 구조라 트리거 빈도도 낮지 않다. 게다가 진단은 `Log` 한 줄(`:84`)뿐이라 표면화되지 않는다. 「파일이 없어도 리셋 후 트래블」은 주석(`:81-82`)대로 의도된 결정이지만, 그 의도가 **파일이 존재하는데 못 읽는 경우**까지 포함하도록 설계된 흔적은 없다.
- **제안**: 이미 있는 `DoesSaveFileExist`(`:170`)로 두 경우를 가른다. 파일이 있는데 로드에 실패했으면 (a) 손상 파일을 `<Slot>.corrupt` 로 옮기거나 활성 슬롯을 임시 이름으로 돌려 덮어쓰기를 차단하고, (b) `Error` 로그와 함께 호출자에게 실패를 알린다. 트래블(월드 리로드) 자체는 지금처럼 이어가도 사망 부활 의미론은 유지된다.
- **확신도**: 중간 (리셋+트래블 폴백은 의도된 설계지만, 손상 파일 덮어쓰기 파급까지 의도된 것으로 보이지 않음)

### 2. 🟡 `FlushPlayerStats` 는 캡처가 0건이면 기존 저장 스탯을 지운다 — 바로 위 `FlushPlayerTransform` 과 방어가 비대칭이다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:196-198` (대조: `:163-169`)
- **범주**: 버그/정확성 (데이터 손실)
- **문제**: `PlayerStats.Reset()` 을 먼저 하고 `CapturePlayerStats` 를 부른 뒤 `bHasPlayerStats = Num() > 0` 로 확정한다. 폰은 있는데 캡처가 0건이면(ASC 없는 폰을 빙의 중 — 탈것·연출 폰·스펙테이터, 또는 `GetSpawnedAttributes()` 가 아직 비어 있는 초기화 창) 이전에 저장돼 있던 스탯이 통째로 사라지고 `bHasPlayerStats=false` 가 되어 로드 후 데이터테이블 기본값으로 되돌아간다. 이 플러시는 곧장 디스크 기록으로 이어지므로 메모리뿐 아니라 파일까지 잃는다. 같은 파일의 `FlushPlayerTransform` 은 동일 상황(폰 부재)을 "이전 캡처 보존" 으로 명시 처리하는데(`:166-169`) 스탯 경로만 그 방어가 없다.
- **제안**: 지역 `TMap` 에 캡처한 뒤 `Num() > 0` 일 때만 `PlayerStats` 를 교체하고 `bHasPlayerStats` 를 갱신한다(실패 시 기존 값 보존 + `Warning`).
- **확신도**: 중간 (현재 플레이어 폰이 ASC 를 직접 소유해 재현 조건이 좁지만, 방어 비대칭 자체는 코드상 명확)

### 3. 🟡 디스크 기록 실패가 로그로만 끝나고 호출자·UI 로 전달되는 경로가 없다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:293-309` · `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h:62` · `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h:38`
- **범주**: 설계/구조 (미처리 실패 경로)
- **문제**: `AsyncSaveGameToSlot` 완료 델리게이트가 성공/실패를 `Log`/`Warning` 으로 찍기만 한다. `SaveToFile` 도 BP 래퍼도 `void` 라, 디스크 풀·권한·클라우드 동기화 잠금으로 기록이 실패해도 UI 는 "저장됨" 으로 진행한다. 게다가 명명 저장 경로는 기록 **전에** 활성 슬롯 정체성을 새 이름으로 바꾸므로(`:138-142`), 한 번 실패한 슬롯을 이후 체크포인트 오토세이브가 계속 목표로 삼아 실패가 누적된다. 세이브 시스템에서 "쓰기 실패를 아무도 모른다" 는 것은 사실상 기능 공백이다.
- **제안**: 서브시스템에 저장 완료 멀티캐스트(슬롯명 + 성공 여부)를 노출하거나 `SaveToFile` 에 완료 콜백 인자를 추가해 UI 가 실패를 표면화하게 한다. 실패 로그는 `Error` 로 승격.
- **확신도**: 높음 (코드 사실. 우선순위는 UI 요구사항에 따라 판단)

### 4. 🟡 월드 서브시스템이 없으면 플러시를 건너뛰고 **스테일 상태를 그대로** 디스크에 쓴다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:156-167`(`:166`) · `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:43-53`(`:52`) · `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp:68-74`
- **범주**: 설계/구조 (권위 모델)
- **문제**: `SaveToFile` 은 월드 서브시스템을 못 찾으면 "플러시할 것이 없으니" 바로 기록한다(`:166`). 그러나 서브시스템 부재는 "플러시할 것이 없다" 와 동치가 아니다. `ShouldCreateSubsystem` 이 **클라이언트 월드를 통째로 배제**하므로(`WxSaveWorldSubsystem.cpp:52`), 클라에서 BP 가 `UWxSaveLibrary::SaveToFile` 을 부르면 라이브 상태가 하나도 반영되지 않은 인메모리 SaveGame 이 클라 디스크에 정상 저장처럼 기록된다. 트랜지션 중 호출도 마찬가지로 직전 스냅샷을 새 저장인 양 남긴다. README 는 "저장은 서버 소유, 클라 진입은 노옵" 을 계약으로 적어 두었지만, 그 게이트가 있는 곳은 ST 태스크(`WxStateTreeTask_SaveGame.cpp:28-32`)뿐이고 BP 진입점에는 없다. 같은 이유로 클라에서 `TravelFromSaveFile` 이 불리면 `bTravelingFromSaveFile` 가 켜진 뒤 이를 내려 줄 월드 서브시스템이 영영 생기지 않아 가드가 세션 내내 스턱된다(`WxSaveGameSubsystem.cpp:116` ↔ `WxSaveWorldSubsystem.cpp:83-86`).
- **제안**: 진입점(서브시스템 또는 라이브러리)에서 권위를 먼저 판정해 클라 호출을 명시적 노옵 + `Warning` 으로 돌린다. 트랜지션 등 "정말 플러시할 게 없는" 경우와 "플러시 대상이 있는데 접근 못 하는" 경우를 로그에서 구분한다.
- **확신도**: 중간 (현재 스탠드얼론 단일 플레이 전제라 미발현. 멀티 진입 시 확정적으로 발현)

### 5. 🟡 `PlayerTransform` 과 `TravelData.Map` 이 원자적으로 갱신되지 않아 맵 게이트가 오탐할 수 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:119` · `:164-169` · `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:210-215`
- **범주**: 버그/정확성
- **문제**: `TryGetPlayerTransform` 의 "저장 맵 == 현재 맵" 게이트는 두 필드가 한 저장에서 같이 쓰였다는 전제 위에 서 있다. 그런데 `FlushMapTravelData` 는 현재 맵을 **무조건** 다시 스탬프하고(`:119`), `FlushPlayerTransform` 은 `ResumeTransform` 도 폰도 없으면 이전 값을 남긴 채 조기 반환한다(`:164-169`). 실패 시나리오: 맵 A 에서 체크포인트 저장 → 같은 세션에서 맵 B 로 이동(teardown 은 트래블 데이터를 건드리지 않는다) → 맵 B 에서 폰이 없는 순간(사망 직후·빙의 전) 명시 저장 → 파일에 `Map=B` + `PlayerTransform=A 좌표` 가 남는다. 이후 로드하면 게이트를 통과해 맵 B 에서 A 좌표로 스폰한다(지오메트리 내부·월드 밖 가능).
- **제안**: 폰 부재로 캡처하지 못했는데 저장 맵이 바뀌었다면 `PlayerTransform` 을 Identity(미설정 sentinel)로 되돌리거나, 재개 지점을 `{Map, Transform}` 한 구조체로 묶어 항상 함께 쓴다.
- **확신도**: 중간 (트리거 조건이 좁다 — 현재 저장 경로는 대부분 폰이 살아 있다)

### 6. 🟡 `bSaveInProgress` 가 저장 요청 단위가 아닌 서브시스템 전역 단일 플래그다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:145`, `:296-299` · `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:64`
- **범주**: 설계/구조 (상태 관리)
- **문제**: `SaveToFile` 이 플래그를 세우고(`:145`) 비동기 기록 콜백이 내리는데(`:298`), 요청을 구분하지 않는다. 저장 A 진행 중 저장 B 가 시작되면 A 의 콜백이 B 의 기록 도중 플래그를 내리고, `IsSaveInProgress()` 를 폴링하는 ST 태스크는 자기 저장이 끝나기 전에 Succeeded 로 빠진다. 반대로 무관한 저장이 진행 중이면 이미 끝난 태스크가 계속 Running 으로 남는다. 데이터 자체는 `AsyncSaveGameToSlot` 이 게임 스레드에서 동기 직렬화 후 쓰기만 비동기로 돌리므로 손상되지 않지만, "저장 완료를 기다린다" 는 태스크 계약은 깨진다.
- **제안**: 진행 중 요청 카운터(`int32 PendingSaveCount`)로 바꾸면 조기 완료는 막힌다. 태스크별 정확성이 필요하면 요청 ID 를 발급해 `IsSaveInProgress(RequestId)` 로 폴링한다.
- **확신도**: 높음 (동시 저장이 실제로 겹치는 빈도는 낮음)

### 7. 🟡 `OnPossessedPawnChanged` 구독이 `OnUnregister` 에서 해제되지 않는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:56` (대조: `:39-45`)
- **범주**: 버그/정확성 (객체 수명주기)
- **문제**: `OnRegister` 가 건 `PostLoginHandle` 은 `OnUnregister` 가 정확히 해제하는데(`:41-42`), `HandleGameModePostLogin` 이 오너 PC 에 건 동적 구독은 어디서도 `RemoveDynamic` 되지 않는다. 이 컴포넌트는 Experience 의 컴포넌트 주입 액션으로 붙고 떨어지는 대상이므로(헤더 `WxPlayerSpawnComponent.h:21`), 컴포넌트만 언레지스터되고 PC 는 살아 있는 전환에서 구독이 남는다. 그 상태로 빙의가 일어나면 이미 떨어진 컴포넌트가 `ApplySavedPlayerStats` 를 실행해 저장 스탯이 현재 폰에 덮인다. 같은 비대칭 탓에 재등록 시 중복 구독도 생긴다(`OnRegister` 의 캐치업 경로가 `HandleGameModePostLogin` 을 다시 부른다).
- **제안**: `OnUnregister` 에서 `GetController<APlayerController>()` 로 `RemoveDynamic(this, &UWxPlayerSpawnComponent::HandlePossessedPawnChanged)` 를 호출해 구독/해제를 대칭으로 맞춘다.
- **확신도**: 중간 (GC 된 오브젝트는 동적 델리게이트가 스스로 정리하므로 실제 문제는 "살아 있지만 언레지스터된" 경우로 한정)

### 8. 🟢 규칙 위반 — 헤더 인라인 함수 정의 2건이 남아 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h:34`, `:37`
- **범주**: 규칙 위반 (CLAUDE.md 코딩 규칙 6 — 인라인 함수 정의 금지)
- **문제**: `GetSaveGame()` 과 `IsTravelingFromSaveFile()` 이 헤더에서 본문째 정의돼 있다. 프로젝트는 이 규칙을 일괄 정리 커밋으로 관리 중이며(`4e2ec381` 헤더 인라인 6건 이관, 최근 `14a77aef` WxUI 이관), 그 스윕이 WxSave 만 놓쳤다 — 당시 이 모듈에서 바뀐 것은 ST 태스크 헤더의 예외 근거 주석 한 줄뿐이다. 같은 파일의 `WxStateTreeTask_SaveGame.h:41` `GetInstanceDataType()` 은 `:13` 에 예외 근거가 명시돼 있어 해당 없음.
- **제안**: 두 게터 본문을 `WxSaveGameSubsystem.cpp` 로 내린다(선언 순서 그대로).
- **확신도**: 높음

### 9. 🟢 규칙 위반 — 델리게이트 콜백의 `Handle` prefix 누락 + 불필요한 람다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:160`, `:292-309`
- **범주**: 규칙 위반 (코딩 규칙 4 — 콜백 `Handle` prefix, 규칙 3 — 불필요한 람다)
- **문제**: (1) `ContinueSaveToFileToDisk` 는 `FOnSaveFlushComplete::FDelegate::CreateUObject` 로 바인딩되는 콜백인데 `Handle` prefix 가 없다(`:160`). 직접 호출(`:166`)을 겸하는 continuation 이라 순수 핸들러와 결이 다르긴 하다. (2) `AsyncSaveGameToSlot` 완료 델리게이트(`:292-309`)는 `TWeakObjectPtr` 를 캡처하는 람다인데, `FAsyncSaveGameToSlotDelegate::CreateUObject(this, &...)` 로 멤버를 바인딩하면 수명 처리가 자동이라 람다도 위크 포인터도 필요 없다. 반면 `Wx.Save.Dump` 의 람다(`:16`)는 정적 초기화 시점이라 멤버 바인딩이 불가능해 정당하다. 나머지 콜백(`HandleWorldInitializedActors`·`HandleGameModePostLogin`·`HandlePossessedPawnChanged`)은 규칙을 지킨다.
- **제안**: `:292` 람다를 `HandleSaveGameToSlotComplete(const FString&, int32, bool)` 멤버로 빼고 `CreateUObject` 로 바인딩한다(항목 3 의 실패 통지도 같은 자리에 얹으면 된다). `ContinueSaveToFileToDisk` 는 델리게이트 전용 얇은 `HandleSaveFlushComplete` 를 두고 그 안에서 부르는 형태가 규칙과 의미를 모두 지킨다.
- **확신도**: 높음

### 10. 🟢 서브시스템/SaveGame 획득 체인이 4개 함수에 그대로 복제돼 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:108-111`, `:125-129`, `:147-151`, `:179-183`
- **범주**: 중복/복잡도
- **문제**: `GetWorld() → GetGameInstance() → GetSubsystem<UWxSaveGameSubsystem>() → GetSaveGame()` 4단 널 체크가 `FlushMapTravelData`·`FlushSavableActors`·`FlushPlayerTransform`·`FlushPlayerStats` 에 반복된다. 실패 처리도 제각각이라(둘은 `Warning`, 둘은 무로그) 일관성이 없고, 항목 2·5 같은 "조용한 조기 반환" 이 눈에 띄지 않는 배경이기도 하다.
- **제안**: private 헬퍼 `UWxSaveGame* GetActiveSaveGame() const` 하나로 접고 로그는 필요한 호출부에만 남긴다.
- **확신도**: 높음

### 11. 🟢 플레이어 스탯 키가 어트리뷰트 프로퍼티 이름뿐이라 AttributeSet 이 둘 이상이 되면 조용히 섞인다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:228`, `:261`
- **범주**: 버그/정확성 (잠재)
- **문제**: 캡처는 `OutStats.Add(It->GetFName(), ...)`, 적용은 `InStats.Find(It->GetFName())` 로 **AttributeSet 클래스를 무시한 평면 이름 키**를 쓴다. 이 순회는 `GetSpawnedAttributes()` 전체를 도는 일반화된 구조이므로, 같은 이름의 어트리뷰트를 가진 두 번째 AttributeSet 이 추가되는 순간 캡처는 뒤에 온 값이 앞 값을 덮고 적용은 한 값이 양쪽에 뿌려진다. 현재는 세트가 하나라 발현하지 않는다.
- **제안**: 키를 `<AttributeSet 클래스명>.<프로퍼티명>` 으로 바꾼다. 기존 슬롯과 호환이 깨지므로 적용 측에 구 평면 키 폴백을 한 버전 남기는 편이 안전하다.
- **확신도**: 중간 (현 시점 미발현, 세트 추가 시 확정적으로 발현)

### 12. 🟢 `ResumeTransform` 원시 포인터가 헤더에 명시된 "비동기 확장 seam" 과 충돌한다
- **위치**: `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h:40`(대조: `:31`) · `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:43-47`
- **범주**: 설계/구조 (객체 수명주기)
- **문제**: `RequestSaveFlush(..., const FTransform* ResumeTransform)` 는 호출자 스택의 임시 객체를 원시 포인터로 받고, ST 태스크는 지역 변수의 주소를 넘긴다(`:44-47`). 현재는 플러시가 전부 동기라 안전하다. 그러나 같은 헤더 `:31` 이 이 델리게이트를 "비동기 작업이 생길 때 지연 완료로 되돌릴 seam" 으로 명시해 두었고, 그 확장을 실제로 하는 순간 `FlushPlayerTransform` 이 파괴된 스택 객체를 역참조한다. 증상은 "재개 지점이 가끔 쓰레기 좌표" 라는 재현 어려운 형태로 나온다.
- **제안**: `const FTransform*` 를 `TOptional<FTransform>` 값 전달로 바꾼다(`WxSaveGameSubsystem.h:62` 포함). 호출부는 3곳뿐이고 의미론은 그대로다.
- **확신도**: 높음 (현재 동작은 정상 — 명시된 확장 지점과의 충돌이 근거)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`(직렬화·버전 헤더·자동 캡처/복원·ASC 스탯 2패스 적용), `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`(슬롯 수명·트래블 가드·비동기 디스크 기록), `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`(StartSpot 주입·스탯 복원 타이밍), `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`(데이터 모델·버전 헤더 설계), `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/README.md`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`·`Private/Gimmick/WxGimmickStateTreeComponent.cpp`(SaveId 부여 경로 확인용)
- **확인했으나 발견 없음**: `WxSave.Build.cs:11-21` 은 `WxCore` + 엔진 모듈만 의존 — 「WxCore 외 Wx 플러그인 참조 금지」 준수. `BlueprintCallable` 은 `UWxSaveLibrary`(BlueprintFunctionLibrary)에만 사용. 전 소스 첫 줄 Copyright, `Wx` prefix, override 의 `Super::` 호출 모두 정상. `FWxStateTreeTask_SaveGame` 이 생성자에서 `bShouldCallTick` 을 세우지 않아도 5.8 의 `FStateTreeTaskCommonBase` 는 기본값을 건드리지 않아(`StateTreeTaskBase.h:25`, `:165-168`) 폴링이 동작한다 — 빈 생성자는 그 사실을 적어 둔 자리다. 레코드 키 충돌(같은 GUID 액터)은 `AWxSpawner::PostDuplicate`·`UWxGimmickStateTreeComponent` 의 등록 시 오너 GUID 대조로 소유 모듈이 이미 막고 있다. `CaptureActor`/`RestoreActor` 의 버전 헤더 왕복, `TMap::FindOrAdd` 참조 수명, 월드 필터링(`Params.World != GetWorld()`), 트래블 가드의 스트리밍-아웃/teardown 스킵 조합도 정확하다.
- **미검토 / 한계**: 실제 세이브 파일 왕복(저장→종료→로드) 실행 검증은 하지 않았다(정적 리뷰). `ST_CheckPoint` 의 태스크 실행 순서(회복 GE 와 `Save Game` 중 무엇이 먼저인지 — 저장되는 HP 를 좌우한다)는 ST 에셋 영역이라 보지 않았고, `UWxSaveLibrary` 를 부르는 사망 화면·명명 슬롯 WBP 호출부도 범위 밖이다. `FObjectAndNameAsStringProxyArchive` 를 `bLoadIfFindFails=false` 로 쓰는 선택은 미로드 오브젝트 참조를 null 로 만들 수 있으나, 현재 프로젝트의 `UPROPERTY(SaveGame)` 이 `bool`·`FGameplayTag` 같은 POD 뿐이라 발현 여지가 없어 발견으로 올리지 않았다. `FlushSavableActors` 의 전 액터 순회(`TActorIterator` + 액터별 `FindComponentByInterface`)는 오픈월드 액터 수가 커지면 저장 1회 비용이 눈에 띌 수 있으나, 매 틱 경로가 아니라 성능 발견으로 세우지 않았다.

---
*문서 기준 커밋 `14a77aef` · 리뷰일 2026-08-03 · 소스 13파일 — `/module-review`로 갱신*
