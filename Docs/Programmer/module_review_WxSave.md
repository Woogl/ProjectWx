# WxSave — 코드 리뷰

> 13파일 1651줄의 작고 잘 정리된 모듈이다. 널 가드·권위 게이트·PIE 격리·직렬화 버전 헤더 등 세이브 시스템의 함정 대부분을 이미 의식적으로 처리했고 헤더 주석이 근거까지 남긴다. 다만 "저장 요청이 겹치는 경우"와 "트래블이 실패하는 경우"라는 두 비정상 경로가 통째로 비어 있다. 이번 리뷰는 전 소스(헤더·cpp 전량)를 읽고, 판정 근거가 되는 엔진 동작(`AsyncSaveGameToSlot`·`UWorld::ServerTravel`·`ISaveGameSystem::SaveGameAsync`)은 UE 5.8 엔진 소스로 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |

## 결과

### 1. 🔴 겹친 `SaveToFile` 이 무방비 — 옛 스냅샷이 파일을 이기고, 완료 신호가 엉뚱한 저장에 발화한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:149`, `:292`, `:311-317`
- **범주**: 버그/정확성
- **문제**: `SaveToFile` 에 재진입 가드가 없다. 진행 중인 기록이 있어도 그대로 두 번째 플러시 + 두 번째 `AsyncSaveGameToSlot` 을 건다. `bSaveInProgress` 는 카운터가 아닌 bool 이고, `FinishSaveInProgress`(:311)는 **먼저 끝난 쪽**의 콜백에서 플래그를 내리고 `OnSaveCompleted` 를 Broadcast + Clear 한다. 결과가 셋이다.
  - 엔진 `ISaveGameSystem::SaveGameAsync` 는 파이프로 병렬 실행만 막을 뿐 **순서를 보장하지 않는다**(엔진 주석: "note that the order is not guaranteed"). 직렬화는 요청 시점에 동기로 끝나므로, 늦게 요청한 체크포인트가 먼저 디스크에 닿고 **더 오래된 스냅샷이 최종 파일로 남을 수 있다** — 세이브 시스템에서 가장 나쁜 실패다.
  - `IsSaveInProgress()` 가 아직 쓰기가 남았는데 false 를 답한다.
  - `FWxStateTreeTask_SaveGame` 이 두 번째 저장을 기다리며 붙었는데 첫 번째 저장의 완료로 깨어나 `Succeeded` 를 반환한다(`WxStateTreeTask_SaveGame.cpp:54`). 반대로 Clear 이후에 붙은 대기자는 자기 저장이 끝나도 신호를 못 받는다.
  - 실제 발생 경로: 체크포인트 ST 태스크 두 개를 짧은 간격으로 통과, 또는 체크포인트 오토세이브와 UI 명명 세이브가 겹치는 경우.
- **제안**: 진행 중이면 새 요청을 거절하거나(경고 + 조기 반환) 큐잉해 직렬화한다. 대기 신호도 저장 요청 단위로 구분되게 바꾼다(요청 ID 또는 요청마다 새 일회성 델리게이트). 최소한 `bSaveInProgress` 를 카운터로 바꾸고 카운트가 0 이 될 때만 Broadcast 한다.
- **확신도**: 높음

### 2. 🟡 트래블이 실패하면 `bTravelingFromSaveFile` 가드가 영구히 걸린 채 남는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:122`, `:126`, `:246`
- **범주**: 버그/정확성
- **문제**: 가드는 `TravelFromSaveFile` 이 세우고, 오직 새 월드의 `UWxSaveWorldSubsystem::OnWorldBeginPlay` → `ReportTravelFromSaveFileComplete`(`WxSaveWorldSubsystem.cpp:83`) 만이 내린다. 새 월드가 뜨지 못하면 내려줄 사람이 없다. 그리고 `UWorld::ServerTravel` 은 실패를 거의 알려주지 않는다 — 엔진 구현상 `NextURL` 이 이미 차 있거나 seamless travel 중이면 **아무것도 하지 않고 true 를 반환**하며, URL 이 접수돼도 맵 로드 실패는 다음 틱의 travel failure 경로에서 일어나 이 코드로 돌아오지 않는다. 가드가 걸린 채 남으면 teardown 플러시와 스트리밍-아웃 캡처가 **세션 내내 조용히 전부 스킵**되어(`WxSaveWorldSubsystem.cpp:514`, `:548`) 이후 저장이 전부 낡은 상태를 기록한다. 로그도 남지 않는다.
- **제안**: `FCoreDelegates`/`GEngine->OnTravelFailure()` 를 구독해 실패 시 가드를 내린다. 또는 가드에 만료 조건(트래블 시작 월드 포인터·프레임/시간 워치독)을 붙여, 기대한 새 월드가 오지 않으면 경고와 함께 해제한다.
- **확신도**: 중간 (정상 경로에선 재현되지 않으며, 실패 시 조용히 나빠지는 유형이다)

### 3. 🟡 BP 진입점에 권위 게이트가 없다 — 클라이언트에서 호출하면 로컬 트래블로 세션을 이탈한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp:68-82` (`SaveToFile`, `TravelFromSaveFile`)
- **범주**: 설계/구조
- **문제**: 모듈의 나머지는 권위를 일관되게 지킨다 — `UWxSaveWorldSubsystem::ShouldCreateSubsystem` 은 `NM_Client` 를 제외하고(`WxSaveWorldSubsystem.cpp:51`), ST 태스크와 스폰 컴포넌트는 `HasAuthority()` 로 막는다. 그런데 BP 파사드인 `UWxSaveLibrary` 만 무게이트다. 클라이언트에서 `TravelFromSaveFile` 을 부르면 `UWorld::ServerTravel` 이 `GetAuthGameMode()==nullptr` 이라 `CanServerTravel` 검사를 건너뛰고 `NextURL` 을 세팅 + `NextSwitchCountdown=0` 으로 두므로, 그 클라이언트가 **서버에서 떨어져 로컬 맵으로 이동**한다. 클라의 `SaveToFile` 도 월드 서브시스템이 없어 플러시 없이 인메모리 슬롯을 그대로 로컬 디스크에 쓴다.
- **제안**: 라이브러리 함수 선두에서 `World->GetNetMode() != NM_Client` (또는 `GetAuthGameMode() != nullptr`)를 확인하고, 아니면 경고 후 noop 한다.
- **확신도**: 중간 (모듈 주석이 "스탠드얼론 싱글 전제"를 여러 곳에 남기고 있어 의도된 미대응일 수 있으나, 같은 모듈의 다른 진입점과 불일치한다)

### 4. 🟡 `ResumeTransform` 이 원시 포인터로 "비동기가 될 예정"인 seam 을 관통한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:41-44` → `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h:62` → `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:26-40`
- **범주**: 설계/구조
- **문제**: ST 태스크는 스택 지역변수 `ResumeTransform` 의 주소를 `SaveToFile` 에 넘기고, 그 포인터는 `RequestSaveFlush` → `FlushPlayerTransform` 까지 그대로 전달된다. 현재는 플러시가 전부 동기라 안전하다. 그런데 `WxSaveWorldSubsystem.h:32-33` 이 이 지점을 "비동기 작업이 생길 때 지연 완료로 되돌릴 seam" 이라고 명시적으로 광고한다 — 그 되돌림이 일어나는 순간 이 포인터는 즉시 댕글링이 되고, 잘못된 좌표로 조용히 재개 지점이 잡힌다(크래시도 아니라 발견이 늦다).
- **제안**: `const FTransform*` 대신 값(`TOptional<FTransform>`)으로 넘긴다. 호출 3단계 모두 시그니처만 바꾸면 되고 비용도 없다.
- **확신도**: 중간 (오늘의 버그는 아니고, 모듈이 스스로 예고한 변경에 대한 선제 방어다)

### 5. 🟢 `StartNewSaveFile` 이 빈 SlotName 을 검증하지 않아 이후 모든 기록이 실패한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:45-66` (특히 `:60`)
- **범주**: 버그/정확성
- **문제**: 헤더(`WxSaveGameSubsystem.h:39-40`)는 "유효한 이름을 넘겨야 한다"고 적었지만 코드는 `SpecificClass` 만 검사하고 SlotName 은 그대로 받는다. `UWxSaveLibrary::StartNewSaveFile` 로 BP 에 노출돼 있어 빈 문자열이 들어오기 쉽고, 그러면 엔진 `AsyncSaveGameToSlot` 의 `SlotName.Len() > 0` 조건에 걸려 이후 모든 저장이 실패한다 — 남는 단서는 매 저장마다의 "디스크 기록 실패" Warning 한 줄뿐이라 원인 추적이 어렵다.
- **제안**: `SlotName.IsEmpty()` 면 다른 실패와 같은 형식의 Warning 을 남기고 nullptr 을 반환한다.
- **확신도**: 높음

### 6. 🟢 규칙 위반 — 불필요한 람다 + 델리게이트 콜백 `Handle` prefix 누락
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:293`, `:163`
- **범주**: 규칙 위반
- **문제**: `:293` 의 `FAsyncSaveGameToSlotDelegate::CreateLambda` 는 `TWeakObjectPtr` 캡처 하나 때문에 쓰였는데, 같은 시그니처의 멤버 함수 + `CreateUObject` 로 그대로 대체된다(약한 수명 처리는 `CreateUObject` 가 이미 해준다) — 코딩 규칙 3(람다는 반드시 필요할 때만) 위반이다. 그 멤버 함수는 규칙 4 에 따라 `HandleSaveGameWritten` 같은 이름이 된다. `:163` 의 `ContinueSaveToFileToDisk` 도 델리게이트에 바인딩되는 콜백인데 `Handle` prefix 가 없다(규칙 4). 같은 파일 `:15` 의 콘솔 명령 람다는 전역 static 등록이라 대체 수단이 없으므로 대상이 아니다.
- **제안**: `:293` 람다를 `HandleSaveGameWritten(const FString&, int32, bool)` 멤버 + `CreateUObject` 로 교체한다. `ContinueSaveToFileToDisk` 는 직접 호출부(`:168`)도 있으니, 델리게이트 바인딩 전용 얇은 `Handle...` 래퍼를 두거나 이름을 `HandleSaveFlushComplete` 로 바꾼다.
- **확신도**: 높음

### 7. 🟢 저장 성공/실패가 대기자에게 전달되지 않아 ST 태스크가 실패에도 `Succeeded` 를 반환한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:293-317`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:54-59`
- **범주**: 설계/구조
- **문제**: 비동기 콜백의 `bSuccess` 는 로그로만 쓰이고 버려진다. `OnSaveCompleted` 는 `FSimpleMulticastDelegate` 라 결과를 실을 자리가 없고, 그래서 체크포인트 ST 태스크는 디스크 기록이 실패해도 `Succeeded` 로 흐른다. 체크포인트 저장이 실패했는데 그래프가 "저장 완료" 로 진행하는 상황을 그래프 쪽에서 분기할 수단이 없다.
- **제안**: `DECLARE_MULTICAST_DELEGATE_OneParam(..., bool /*bSuccess*/)` 로 바꾸고 ST 태스크가 실패 시 `Failed` 를 반환하게 한다. 그래프 저작에 분기를 강요하지 않으려면 최소한 실패를 Error 레벨 로그로 승격한다.
- **확신도**: 중간 (체크포인트는 실패해도 진행하는 편이 낫다는 판단일 수 있다)

### 8. 🟢 저장 스탯 적용 구독이 일회성이 아니라, 이후의 모든 빙의에서 재적용된다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:53`, `:90-97`
- **범주**: 설계/구조
- **문제**: `OnPossessedPawnChanged` 구독은 한 번 붙으면 세션 내내 유지되고, 매 빙의마다 `ApplySavedPlayerStats` 가 **마지막 저장 시점의** 어트리뷰트 스냅샷을 새 폰에 덮어쓴다. 지금은 플레이어 재빙의 경로가 없어(리포 전역에 `Possess()` 호출 없음, 사망 부활은 맵 리로드 경유) 실제로 트리거되지 않는다. 다만 탈것·포탑·조종 대상 전환 같은 오픈월드 액션 RPG 의 흔한 확장이 들어오는 순간, 원래 폰으로 돌아올 때 체크포인트 시점 HP 등으로 되감기는 회귀가 된다.
- **제안**: 첫 유효 빙의에서 적용한 뒤 구독을 떼거나(`RemoveDynamic`), 적용 여부 플래그로 1회로 제한한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — "스탠드얼론 싱글·단일 폰 전제" 주석이 모듈 전반에 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/WxSave.uplugin`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` (계약 확인용, 리뷰 대상 아님)
- **미검토 / 한계**:
  - 모듈 규칙(플러그인 참조·prefix·Copyright 헤더) 준수는 전량 확인했고 위반 없음 — `WxSave.Build.cs` 의 Wx 의존은 `WxCore` 하나뿐이다. `WxStateTreeTask_SaveGame.h:42` 의 인라인 `GetInstanceDataType()` 은 코딩 규칙 6 에 대한 형식적 예외지만 `:13` 에 근거가 명시돼 있고 엔진 StateTree 의 강제 형태라 발견으로 올리지 않았다.
  - 직렬화 정확성(`CaptureActor`/`RestoreActor` 의 버전 헤더 왕복, 아키타입 대비 `ShouldSave` 판정)은 코드 독해로만 검증했고 실제 이기종 빌드 왕복 테스트는 하지 않았다. 로직상 결함은 찾지 못했다.
  - `WAS_CoreGameplay` 등 Experience 에셋의 `UWxPlayerSpawnComponent` 주입 등록 여부는 BP/에셋 내부라 범위 밖 — 참조가 존재한다는 것만 확인했다.
  - World Partition 스트리밍 실환경에서의 `LevelAddedToWorld`/`LevelRemovedFromWorld` 발화 타이밍은 정적 분석만 했다.

---
*문서 기준 커밋 `13b45192` · 리뷰일 2026-08-25 · 소스 13파일 — `/module-review`로 갱신*
