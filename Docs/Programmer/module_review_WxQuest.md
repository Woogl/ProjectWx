# WxQuest — 코드 리뷰

> 15파일짜리 소형 모듈로, 러너 소유·권위·재진입·에셋 불가지라는 설계 전제가 클래스 doc-comment에 명시돼 있고 코드가 그 전제를 대체로 지킨다 — 심각 결함은 없고, 남은 문제는 대부분 "조용한 실패"(로그·진단 부재) 계열이다. 이번 리뷰는 `WxQuest.Build.cs`·`WxQuest.uplugin`과 Public/Private 전 소스 15개를 읽고, 소비자 쪽(`Source/WxGame/MVVM/WxViewModel_Quest.cpp`)과 컴포넌트 주입 경로(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`)까지 따라가 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 StartNextQuest 만 조립 오류를 아무 흔적 없이 삼킨다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:24-27`
- **범주**: 버그/정확성
- **문제**: 헤더 doc-comment(`Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h:27`)는 "퀘스트 컴포넌트가 없으면 … 경고를 남기고 예약하지 않는다"고 적혀 있으나 cpp 에는 `UE_LOG` 가 없다. 같은 상황에서 형제 태스크 둘은 경고를 남기므로(`WxStateTreeTask_SetQuestTitle.cpp:26-28`, `WxStateTreeTask_SetQuestObjective.cpp:26-28`) 4종 중 이 하나만 규약을 어긴다. 실제로 이 파일만 `WxQuestModule.h` 를 include 하지 않아(`:5-8`) 로그를 빼먹은 것이 명백하다.

  결과적으로 퀘스트 러너 밖 조립·컴포넌트 부재 상황에서 **퀘스트 체인이 다음 퀘스트로 넘어가지 않은 채 아무 단서 없이 끝난다**. 체인 종점(빈 지정)과 조립 오류가 육안으로 구분되지 않아 원인 추적이 불가능하다.

  덧붙여 여기서 반환하는 `EStateTreeRunStatus::Failed` 는 세 태스크 모두 생성자에서 `bConsideredForCompletion = false` 를 켜 두었으므로(`:14-17`) 상태 완료 판정에 반영되지 않을 가능성이 높다 — 즉 실패가 상태로도 드러나지 않는다.
- **제안**: 형제 태스크와 같은 형식의 `UE_LOG(LogWxQuest, Warning, ...)` 을 추가하고 `WxQuestModule.h` 를 include 한다. `Failed` 반환은 완료 판정에서 빠진 태스크에선 사실상 죽은 값이므로, 진단을 로그에만 의존한다는 점을 세 태스크의 doc-comment에 명시해 의도를 못 박는 편이 낫다.
- **확신도**: 높음 (로그 부재는 확실. `Failed` 폐기 여부는 엔진 소스를 이 환경에서 대조하지 못해 중간)

### 2. 🟡 BP 공개 진입점이 재진입 안전 경로를 우회한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h:23-25`
- **범주**: 설계/구조
- **문제**: 컴포넌트는 재진입 위험한 `ActivateQuest` 와 안전한 `RequestActivateQuest`(다음 틱 예약) 두 진입점을 갖는데(`WxQuestComponent.h:54,57`), BP 에서 부를 수 있는 것은 `UWxQuestLibrary::StartQuest` → `ActivateQuest` 직결 경로 하나뿐이다. 이 노드가 러너 실행 콜스택 안(퀘스트 ST 노드가 호출한 BP, ST 로 구동되는 기믹 등)에서 한 번이라도 불리면 엔진이 Running 중 `SetStateTreeReference` 를 거부하는 사이 `StopLogic` 은 이미 러너를 정지시키고 `HandleStateTreeRunStatusChanged` → `ClearJournal` 로 저널까지 비운 뒤라, **진행 중이던 퀘스트가 사라지고 새 퀘스트도 시작되지 않는 복구 불가 상태**로 끝난다. "콜스택 밖에서만"이라는 호출부 규약이 코드가 아니라 doc-comment로만 강제되는데, 정작 `WxQuestLibrary.h:11-16` 의 doc-comment에는 그 경고가 없다.

  같은 함수는 실패 경로도 전부 무진단이다 — GameState 에 컴포넌트가 없거나(Experience 주입 누락) 비-권위 머신이면 `if` 하나로 조용히 노옵이 된다. 레벨에 트리거 볼륨을 놓은 기획자 입장에선 "아무 일도 안 일어남"만 남는다.
- **제안**: `StartQuest` 를 `RequestActivateQuest` 로 라우팅해 호출 위치와 무관하게 항상 다음 틱 예약으로 만든다(하드 포인터를 `TSoftObjectPtr` 로 감싸 그대로 넘길 수 있다). 즉시 시작이 필요한 곳만 C++ 에서 `ActivateQuest` 를 직접 부르면 규약이 코드로 보장된다. 아울러 컴포넌트 부재 시 `LogWxQuest` 경고를 하나 남긴다.
- **확신도**: 중간

### 3. 🟡 다음 틱 활성화 경로의 널 역참조와 중복 예약
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:40-49`
- **범주**: 버그/정확성
- **문제**: 두 가지가 겹쳐 있다.
  1. `:48` — `GetWorld()->GetTimerManager()` 를 널 검사 없이 역참조한다. 현재 유일한 호출부(`StartNextQuest::EnterState`)에선 월드가 항상 유효하지만, 이 함수는 헤더에 공개된 API 라 월드 해체 중·컴포넌트 미등록 상태 호출에 무방비다.
  2. 예약을 합류시키지 않는다. 같은 프레임에 `RequestActivateQuest` 가 두 번 이상 불리면(병렬 상태에 `StartNextQuest` 가 둘, 또는 체인과 트리거 볼륨이 같은 프레임에 겹침) 타이머가 각각 쌓이고, 다음 틱에 순서대로 `ActivateQuest` 가 돌아 **중간 퀘스트가 진입 → 곧바로 정지**된다. 저널은 마지막 것만 남지만, 진입 시점에 이미 실행된 부수효과(스폰·보상 등 크로스모듈 ST 노드)는 되돌아가지 않는다.
- **제안**: `GetWorld()` 널 가드를 추가하고, 대기 중인 요청을 멤버 1개(`PendingQuest` + `FTimerHandle`)로 유지해 같은 프레임의 재요청은 마지막 것만 살린다.
- **확신도**: 중간

### 4. 🟢 `HasActiveQuest()` 가 실제로 뜻하는 것은 "저널이 채워졌는가"다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:51-57`, `:84-87`
- **범주**: 설계/구조
- **문제**: `bHasActiveQuest` 를 `true` 로 만드는 곳은 `SetQuestTitle`(`:55`) 하나뿐이고 `ActivateQuest` 는 이 플래그를 건드리지 않는다. 따라서 러너는 돌고 있으나 아직 제목 태스크가 진입하지 않은 구간, 또는 `SetQuestTitle` 태스크를 배치하지 않은 퀘스트는 이름과 반대로 "활성 퀘스트 없음"을 보고한다. 현재 유일한 소비자는 HUD 게이트라 문제가 드러나지 않지만(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:50`), 이름만 보고 "퀘스트 진행 중 판정"으로 재사용하는 두 번째 소비자가 생기면 어긋난다.
- **제안**: 저널 표시용이 의도라면 이름을 의미에 맞게(예: `HasJournalEntry`) 바꾸고, 이름을 유지하려면 `ActivateQuest` 성공 시점에 플래그를 세운 뒤 제목 유무는 별도로 다룬다.
- **확신도**: 중간 (표시 전용 플래그라면 의도된 설계일 수 있음)

### 5. 🟢 런타임 생성한 러너를 컴포넌트 파괴 시 정리하지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:106-121`
- **범주**: 버그/정확성
- **문제**: `BeginPlay` 가 `UStateTreeComponent` 를 GameState 를 Outer 로 만들어 등록하지만(`:117-120`), 이 파일에는 `EndPlay`·`OnUnregister` 가 전혀 없다. 러너의 Outer 는 컴포넌트가 아니라 GameState 이므로, 퀘스트 컴포넌트만 파괴되면 러너는 **등록·틱 상태 그대로 남아 퀘스트 StateTree 를 계속 돌린다** — 저널을 갱신할 상대는 사라졌는데 월드 부수효과만 계속 나는 상태다. 재주입되면 `NewObject` 가 같은 이름 `TEXT("QuestStateTree")` 로 두 번째를 만들려다 기존 오브젝트와 충돌한다(`:117`).

  컴포넌트 단독 파괴는 GameFeature 비활성화 시 `ComponentRequestHandles` 해제로 일어나는데(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:67-78`), 현재 그 시점은 `UWxExperienceManagerComponent::EndPlay` 뿐이라 GameState 도 같이 죽는다 — 그래서 지금은 잠복 상태다. 다만 런타임 Experience 교체나 GameFeature 개별 토글이 생기는 순간 바로 터진다. 같은 모듈의 다른 컴포넌트들은 이 정리를 이미 하고 있다(`Source/WxGame/Character/WxMetaHumanComponent.cpp:125-166`).
- **제안**: `EndPlay`(또는 `OnUnregister`)를 오버라이드해 `OnStateTreeRunStatusChanged` 구독을 끊고 `QuestStateTree->DestroyComponent()` 후 `nullptr` 로 비운다. 대기 중인 다음 틱 타이머 핸들도 같은 자리에서 지운다.
- **확신도**: 중간 (현재 실행 경로에선 재현되지 않는 잠복 결함)

### 6. 🟢 퀘스트 체인 전환마다 게임 스레드 블로킹 로드
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:131-134`
- **범주**: 성능/안전
- **문제**: `HandleDeferredActivateQuest` 가 `LoadSynchronous()` 로 다음 퀘스트 에셋을 받는다. 소프트 참조를 쓰는 이유는 "타이머 대기 중 GC 로 로드가 풀릴 수 있어서"(`:47`)인데, 실행 시점 로드가 동기라 체인이 넘어갈 때마다 게임 스레드가 멈춘다. 퀘스트 ST 에셋이 크로스모듈 노드·텍스트를 물고 있으면 히치가 눈에 띈다.
- **제안**: `RequestActivateQuest` 시점에 `FStreamableManager::RequestAsyncLoad` 로 예열하고, 완료 콜백에서 `ActivateQuest` 를 부른다(예약 합류는 발견 3의 `PendingQuest` 멤버와 같은 자리에서 처리된다).
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, 모듈 밖 대조용 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`
- **규칙 대조 결과(위반 없음)**: 모듈 의존은 `WxCore` + 엔진 플러그인뿐(`WxQuest.Build.cs:11-26`), 전 소스·`Build.cs` 첫 줄 저작권 표기 존재, 람다 0건, `FORCEINLINE` 0건, `BlueprintCallable` 은 BP Function Library 인 `UWxQuestLibrary` 한 곳뿐(`WxQuestLibrary.h:24`), 델리게이트 콜백 2종 모두 `Handle` 접두, `BeginPlay` 의 `Super::` 호출 확인(`WxQuestComponent.cpp:108`). 4개 태스크 헤더의 `GetInstanceDataType()` 인라인 정의는 코딩 규칙 6 이 명문화한 예외이며 요구되는 예외 사유 주석도 각 헤더에 달려 있다(예: `WxStateTreeTask_SetQuestTitle.h:12`) — 위반 아님.
- **미검토 / 한계**: 이 샌드박스에 언리얼 엔진 소스가 없어 StateTree 완료 판정(`bConsideredForCompletion`)·`UStateTreeComponent` 재진입 가드의 정확한 동작을 엔진 코드로 대조하지 못했다 — 발견 1·2·3 의 엔진 측 전제는 API 계약과 코드 주석에 근거한 추론이다. 퀘스트 `UWxQuestStateTree` 에셋의 실제 저작 내용(상태 구조·태스크 배치)은 BP/에셋 영역이라 보지 않았으므로, 발견 1·3 의 "잘못된 조립"·"같은 프레임 중복 예약"이 현 에셋에서 재현되는지는 확인하지 못했다. `FWxStateTreeTask_WaitMoveToTarget` 의 매 틱 `SyncFind` 와 `FVector::Dist` 3D 판정은 헤더(`:34-35`)에 근거가 명시돼 있고 활성 퀘스트가 1개뿐이라 문제로 보지 않았다. `WxQuest.Build.cs:16` 의 `GameplayTags` 는 모듈 안에서 쓰이는 곳이 없으나(사용 0건) 리뷰 항목으로 세울 만큼의 무게는 아니라 여기에만 적는다.

---
*문서 기준 커밋 `cf3a7a0` · 리뷰일 2026-08-25 · 소스 15파일 — `/module-review`로 갱신*
