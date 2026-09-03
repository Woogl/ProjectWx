# WxQuest — 코드 리뷰

> 14파일 규모의 작은 모듈이고 규칙 위반은 하나도 없다. 러너를 권위에서만 띄우는 경계, 저널 정리를 RunStatus 한 곳으로 수렴시킨 설계 모두 깔끔하다. 다만 "실패했을 때 조용히 아무 일도 안 일어나는" 경로가 몇 군데 있어 데이터 주도 저작에서 진단이 어렵다. 이번 리뷰는 전 소스(14파일)를 읽었고, 판단 근거가 되는 엔진 동작(`UStateTreeComponent`의 StopLogic/SetStateTreeReference 재진입 가드, `bConsideredForCompletion` 마스킹)은 UE 5.8 엔진 소스로 직접 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `bConsideredForCompletion = false` 태스크의 `Failed` 반환은 엔진이 통째로 무시한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:23-26`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:28`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:28`
- **범주**: 버그/정확성
- **문제**: 세 태스크 모두 생성자에서 `bConsideredForCompletion = false`를 켜 놓고(각 cpp `:14-17`), 퀘스트 컴포넌트를 못 찾으면 `EStateTreeRunStatus::Failed`를 돌려준다. 그런데 엔진의 `FStateTreeExecutionContext::EnterState`는 `if (CurrentStateTasksStatus.IsConsideredForCompletion(StateTaskIndex))` 안에서만 Failed를 전파한다(UE 5.8 `StateTreeExecutionContext.cpp:3872-3879`). 즉 이 태스크들이 낸 Failed는 마스크 밖이라 상태를 실패시키지 못하고 트리는 아무 일 없었다는 듯 계속 돈다. `WxStateTreeTask_StartNextQuest.h:27`의 "예약 없이 Failed 로 끝난다"는 실동작과 어긋난다. 게다가 `StartNextQuest`는 이 경로에 `UE_LOG`조차 없어(제목·목표 태스크는 Warning을 남긴다) 퀘스트 체인이 완전히 무음으로 끊긴다.
- **제안**: 최소한 `StartNextQuest`에도 다른 두 태스크와 같은 Warning 로그를 넣는다. 반환값으로 상태를 실패시키고 싶다면 이 태스크만 `bConsideredForCompletion`을 유지하거나, `Context.FinishTask(...)`/명시적 전이 등 마스크와 무관한 경로로 실패를 알린다.
- **확신도**: 높음

### 2. 🟡 빈 `Quest` 소프트 참조는 "체인 종점"이 아니라 그냥 노옵이라 마지막 퀘스트가 영원히 남는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:28-30`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:41-44`
- **범주**: 버그/정확성
- **문제**: 주석은 "빈 지정은 컴포넌트가 무시하므로 체인 종점 처리도 같은 호출로 수렴한다"고 안내하지만, `RequestActivateQuest`는 `QuestAsset.IsNull()`이면 즉시 반환할 뿐 러너를 정지시키지 않는다. 저널 정리는 오직 `HandleStateTreeRunStatusChanged`(`WxQuestComponent.cpp:122-128`)에서만 일어나므로, 마지막 상태가 `StartNextQuest`(빈 참조)만 들고 있으면 트리는 Running으로 남고 제목·목표가 HUD에 영구히 붙는다. 이 상태가 스스로 끝날 수도 없다 — 상태의 태스크가 전부 완료 판정에서 빠지면 `CompletionTasksMask == 0`이 되고, 기본 `TasksCompletion = Any`에서 `HasAnyCompleted()`가 항상 false다(UE 5.8 `StateTreeTasksStatus.h:153-156`, `StateTreeState.h:428`). 엔진 주석도 "the mask is 0 … The state tree will never complete"라고 못 박는다(`StateTreeTasksStatus.cpp:112`).
- **제안**: 빈 참조를 진짜 종점으로 쓰려면 `RequestActivateQuest`(또는 다음 틱 콜백)에서 널일 때 `QuestStateTree->StopLogic()`을 태워 저널 정리 경로로 수렴시킨다. 종점 표현이 아니라면 헤더/인라인 주석에서 "체인 종점" 문구를 걷어내고, 종점은 트리 자체 완료로 처리하도록 안내한다.
- **확신도**: 중간

### 3. 🟡 BP 진입점 `StartQuest`가 재진입 위험한 `ActivateQuest`를 그대로 노출한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:32-36`
- **범주**: 설계/구조
- **문제**: `UWxQuestComponent::ActivateQuest`는 "ST 실행 콜스택 밖에서만 호출"이라는 전제를 헤더(`WxQuestComponent.h:54`)에만 적어 두었는데, 유일한 BlueprintCallable 진입점인 `UWxQuestLibrary::StartQuest`는 이를 아무 보호 없이 직접 호출한다. 퀘스트 트리 안에서 실행되는 BP 태스크가 이 노드를 밟으면 `StopLogic`은 재진입 컨텍스트로 지연 처리되고, 이어지는 `SetStateTreeReference`는 "Trying to change the state tree on a running instance" 경고와 함께 거부되며(`StateTreeComponent.cpp:491-501`), `StartLogic`은 "Reentrant call … is not allowed" 에러로 끝난다(`:181-185`). 결과는 현재 퀘스트만 죽고 새 퀘스트는 시작되지 않는 상태 — 엔진 로그 말고는 아무 신호가 없다.
- **제안**: `StartQuest`를 `RequestActivateQuest`(다음 틱 지연)로 위임한다. 지연 1틱은 트리거 볼륨 수주에서 체감되지 않고, 콜스택 안/밖 어느 경로에서 불려도 안전해진다. 즉시 실행이 꼭 필요하면 `ActivateQuest`를 라이브러리에서 노출하지 않는 편이 낫다.
- **확신도**: 중간

### 4. 🟡 저널이 권위 전용이라 원격 클라이언트의 퀘스트 HUD가 비어 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:95-102`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:105-120`
- **범주**: 설계/구조
- **문제**: `QuestTitle`·`Objectives`·`bHasActiveQuest`는 복제 지정자가 없고 `GetLifetimeReplicatedProps`도 없다. 러너는 `HasAuthority()` 게이트로 권위에만 생기므로(`cpp:109-113`) 클라 GameState의 컴포넌트는 영구히 빈 저널이다. 그런데 소비자 `UWxViewModel_Quest`는 클라에서도 GameState에서 컴포넌트를 찾아 붙는다(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:63-76`) — 구독은 되지만 `OnJournalChanged`가 영원히 오지 않아 제목·목표가 빈 채로 남는다. 프로젝트가 락온·기믹 State 등에서 서버 권위 복제를 갖춘 것과 비교하면 퀘스트만 빠져 있다.
- **제안**: 데디/리모트 클라를 지원할 시점에 `FWxQuestObjective`(이미 `UPROPERTY()`가 붙어 있다)와 제목·플래그를 `Replicated`로 올리고 `OnRep`에서 `OnJournalChanged`를 쏜다. v1 범위 밖이라면 클라에서 뷰모델 리졸버가 붙지 않도록 하거나, 최소한 제약을 README 「경계」에 한 줄 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 클래스 doc-comment와 README가 "싱글/리슨 호스트" 전제를 명시한다)

### 5. 🟢 `StartQuest`의 실패 경로가 전부 무음이다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:10-18`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:20-29`
- **범주**: 설계/구조
- **문제**: 에셋 null, GameState에 컴포넌트 없음, 비-권위 호출 세 경우 모두 로그 없이 반환한다. 레벨에 배치한 트리거 볼륨에서 부르는 디자이너용 진입점인데, 퀘스트가 안 뜰 때 원인을 가릴 단서가 없다. 태스크들이 같은 상황에서 `LogWxQuest` Warning을 남기는 것과도 일관되지 않는다.
- **제안**: 최소한 "컴포넌트를 못 찾음"과 "에셋이 null"에는 `LogWxQuest` Warning을 남긴다. 비-권위 노옵은 정상 경로이므로 Verbose면 충분하다.
- **확신도**: 높음

### 6. 🟢 오너에서 퀘스트 컴포넌트를 찾는 3줄이 4곳에 복제돼 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:22-23`, `.../WxStateTreeTask_SetQuestObjective.cpp:22-23`, `.../WxStateTreeTask_SetQuestObjective.cpp:41-42`, `.../WxStateTreeTask_StartNextQuest.cpp:21-22`
- **범주**: 중복/복잡도
- **문제**: `Cast<AActor>(Context.GetOwner())` → `FindComponentByClass<UWxQuestComponent>()` 관용구가 4번 반복된다. 새 퀘스트 태스크를 추가할 때마다 늘어나고, 실패 시 로그 문구도 각자 관리해야 한다(그래서 5번 항목 같은 편차가 생긴다).
- **제안**: `UWxQuestComponent`에 `static UWxQuestComponent* FindQuestComponent(const UObject* Owner)`를 두고 태스크들이 이를 쓰게 한다. 진단 로그도 한 곳으로 모인다.
- **확신도**: 높음

### 7. 🟢 `RequestActivateQuest`가 `GetWorld()`를 검증 없이 역참조한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:47`
- **범주**: 성능/안전
- **문제**: `GetWorld()->GetTimerManager()`에 널 검사가 없다. 현재 호출자가 러너 실행 중인 ST 태스크뿐이라 실제 재현은 어렵지만, 이 함수는 public이고 컴포넌트가 월드에서 떨어진 뒤(오너 파괴 진행 중) 불리면 그대로 크래시다. 같은 파일의 다른 경로들은 모두 방어적으로 작성돼 있어 여기만 튄다.
- **제안**: `UWorld* World = GetWorld(); if (!World) { return; }` 한 줄을 추가한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_*.h`(4개), `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`, 소비자 확인용 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`
- **규칙 점검 결과**: 위반 없음. 14개 소스 전부 `// Copyright Woogle. All Rights Reserved.`로 시작하고, 의존은 `WxCore` + 엔진 모듈뿐이며(`WxQuest.Build.cs`, `WxQuest.uplugin`), 델리게이트 콜백은 `Handle` 접두사를 지키고(`HandleStateTreeRunStatusChanged`, `HandleDeferredActivateQuest`), `BlueprintCallable`은 BP Function Library인 `UWxQuestLibrary`에만 있으며, 헤더 인라인 정의는 `GetInstanceDataType()` 4건뿐으로 모두 예외 사유 주석이 달려 있다. 람다도, `Super::` 누락도 없다.
- **미검토 / 한계**: 퀘스트 `UStateTree` 에셋의 실제 저작 형태(어느 상태에 어떤 태스크가 붙어 있는지)는 확인하지 않았다 — 2번 항목의 "종점 상태가 실제로 그렇게 저작돼 있는가"는 에셋을 열어봐야 확정된다. `FStateTreeExecutionContext::Start`가 트리 교체 시 이전 인스턴스 데이터를 어디까지 리셋하는지는 호출 흐름만 확인했고 끝까지 추적하지 않았다.

---
*문서 기준 커밋 `c486a5c7` · 리뷰일 2026-09-03 · 소스 14파일 — `/module-review`로 갱신*
