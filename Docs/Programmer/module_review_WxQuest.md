# WxQuest — 코드 리뷰

> 소스 15개짜리 소형 모듈로, 러너 소유·권위·재진입·에셋 불가지라는 설계 전제가 doc-comment에 명시돼 있고 코드가 그 전제를 지킨다 — 심각 결함은 없고 남은 문제는 대부분 "조용한 실패"(진단 부재·복구 불가 상태) 계열이다. 이번 리뷰는 `WxQuest.uplugin`·`WxQuest.Build.cs`와 Public/Private 전 소스 15개를 읽고, 소비자(`Source/WxGame/MVVM/WxViewModel_Quest.cpp`)·주입 경로(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.*`)와 **UE 5.8 엔진 소스(StateTree/GameplayStateTree)** 까지 대조해, 이전 리뷰가 추론으로 남겼던 엔진 측 전제 2건을 확증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 BP 공개 진입점이 재진입 안전 경로를 우회한다 — 퀘스트가 통째로 유실된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h:11-25`
- **범주**: 설계/구조
- **문제**: 컴포넌트는 재진입에 취약한 `ActivateQuest` 와 안전한 `RequestActivateQuest`(다음 틱 예약) 두 진입점을 갖는데(`WxQuestComponent.h:54-58`), BP 에 노출된 유일한 노드 `UWxQuestLibrary::StartQuest` 는 `ActivateQuest` 로 직결된다. "ST 실행 콜스택 밖에서만 호출"이라는 규약이 `WxQuestComponent.h:54` 의 주석으로만 존재하고, 정작 BP 사용자가 보는 `WxQuestLibrary.h:11-16` 의 doc-comment 에는 그 경고가 없다.

  이 노드가 러너 콜스택 안(퀘스트 ST 노드가 부르는 BP, ST 로 구동되는 기믹/대화 등)에서 한 번이라도 불리면 엔진 동작상 다음 순서로 **복구 불가 상태**가 된다. UE 5.8 소스로 확인한 결과다.
  1. `StopLogic` → 재진입 컨텍스트로 `Context.Stop()` → `FStateTreeExecutionContext::Stop` 이 지연 처리되며 `Running` 을 반환한다(`StateTreeExecutionContext.cpp:1707-1715`). 반환값이 이전 상태와 같으니 `OnStateTreeRunStatusChanged` 는 이 시점에 발화하지 않는다.
  2. `SetStateTreeReference` 는 실행 상태가 아직 `Running` 이라 **거부**된다(`StateTreeComponent.cpp:491` 이하, "Trying to change the state tree on a running instance").
  3. `StartLogic` → `StartTree` 도 `CurrentlyRunningExecContext` 가 살아 있어 **거부**된다("Reentrant call ... is not allowed").
  4. 진행 중이던 틱이 끝나며 지연된 Stop 이 실행되어 `Stopped` 가 브로드캐스트 → `HandleStateTreeRunStatusChanged` → `ClearJournal`.

  결과: 진행 중이던 퀘스트는 정지·저널 초기화되고 새 퀘스트는 시작되지 않으며, 남는 흔적은 엔진 로그 두 줄뿐이다.
- **제안**: `StartQuest` 를 `RequestActivateQuest` 로 라우팅해 호출 위치와 무관하게 항상 다음 틱 예약으로 만든다(하드 포인터는 `TSoftObjectPtr` 로 감싸 그대로 넘길 수 있다). 즉시 시작이 필요한 곳만 C++ 에서 `ActivateQuest` 를 직접 부르면 규약이 주석이 아니라 코드로 보장된다. 겸사겸사 컴포넌트 부재·비-권위로 노옵되는 경로에 `LogWxQuest` 경고를 하나 남긴다 — 지금은 기획자에게 "아무 일도 안 일어남"만 남는다.
- **확신도**: 높음 (엔진 동작은 소스로 확증. 실제로 콜스택 안에서 호출되는 에셋이 있는지는 BP/에셋 영역이라 미확인)

### 2. 🟡 도달 대기 태스크가 잘못된 대상에도 영원히 Running 을 반환한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23-28`, `:48-54`
- **범주**: 버그/정확성
- **문제**: `EnterState` 는 로케이터가 비어 있으면 경고를 남기지만 그대로 `Running` 을 반환하고(`:23-28`), `Tick` 은 `SyncFind` 가 null 이면 무조건 `Running` 을 유지한다(`:48-54`). 헤더(`WxStateTreeTask_WaitMoveToTarget.h:32`)도 빈 로케이터를 "완료될 수 없는 잘못된 조립"이라고 인정한다.

  문제는 파급 범위다. 같은 상태에 놓이는 제목·목표·체인 태스크는 전부 `bConsideredForCompletion = false` 라 상태를 끝내지 못하므로(각 cpp `:14-17`), 이 Wait 태스크가 상태 완료를 낼 **유일한** 주체다. 따라서 빈 로케이터 하나로 그 상태는 물론 퀘스트 체인 전체가 무기한 정지하며, 활성 퀘스트가 동시 1개라는 전제 때문에 다른 퀘스트로 빠져나갈 길도 없다. 대상 액터가 삭제·리네임되어 `SyncFind` 가 계속 null 인 경우도 결말이 같은데 이쪽은 로그조차 없어, 스트리밍 아웃으로 잠시 미해석인 정상 상황과 육안 구분이 안 된다.
- **제안**: 빈 로케이터는 `EnterState` 에서 `Failed` 를 반환한다 — 이 태스크는 `bConsideredForCompletion` 이 기본값 `true` 라 반환이 상태 전이로 실제 반영되며(발견 4 참조), 조립 오류가 로그가 아니라 트리 흐름으로 드러난다. 미해석 대상은 대기를 유지하되 일정 시간 경과 후 1회만 경고를 남겨 "잠시 언로드"와 "영영 없음"을 구분할 수 있게 한다.
- **확신도**: 높음

### 3. 🟡 다음 틱 활성화 예약이 합류되지 않고, 월드 널 가드가 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:40-49`
- **범주**: 버그/정확성
- **문제**: 두 가지가 겹쳐 있다.
  1. **예약 미합류** — `SetTimerForNextTick` 을 핸들 보관 없이 걸기만 한다(`:48`). 같은 프레임에 요청이 둘 이상 들어오면(병렬 상태에 `StartNextQuest` 가 둘, 또는 체인 예약과 트리거 볼륨의 즉시 시작이 겹침) 다음 틱에 `ActivateQuest` 가 순서대로 돌아 **중간 퀘스트가 진입했다가 곧바로 정지**된다. 저널은 마지막 것만 남지만 진입 시점에 이미 실행된 부수효과(스폰·보상 등 크로스모듈 ST 노드)는 되돌아가지 않는다. 더 흔한 형태는 반대 방향이다 — 플레이어가 체인 예약이 걸린 프레임에 트리거 볼륨을 밟으면, 즉시 시작된 트리거 퀘스트를 다음 틱의 묵은 예약이 조용히 덮어쓴다.
  2. **널 가드 부재** — `GetWorld()->GetTimerManager()` 를 검사 없이 역참조한다(`:48`). 현재 유일한 호출부에선 월드가 유효하지만, 헤더에 공개된 API 라 월드 해체 중 호출에 무방비다. 이 파일의 다른 경로는 전부 가드를 두고 있어 일관성도 깨진다.
- **제안**: 대기 중인 요청을 멤버 하나(`FTimerHandle` + 대기 에셋)로 유지해, 새 요청이나 `ActivateQuest` 진입 시 기존 예약을 `ClearTimer` 로 취소한다 — 같은 프레임의 마지막 의도만 살아남는다. `GetWorld()` 널 가드도 같은 자리에서 추가한다.
- **확신도**: 중간

### 4. 🟡 태스크 4종 중 StartNextQuest 만 조립 오류를 아무 흔적 없이 삼킨다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:22-27`
- **범주**: 버그/정확성
- **문제**: 헤더 doc-comment(`WxStateTreeTask_StartNextQuest.h:27`)는 "퀘스트 컴포넌트가 없으면 … 경고를 남기고 예약하지 않는다"고 적혀 있으나 cpp 에는 `UE_LOG` 가 없다. 형제 태스크 둘은 같은 상황에서 경고를 남기므로(`WxStateTreeTask_SetQuestTitle.cpp:26-28`, `WxStateTreeTask_SetQuestObjective.cpp:26-28`), 4종 중 이 하나만 규약을 어긴다 — 이 파일만 `WxQuestModule.h` 를 include 하지 않은 것(`:3-8`)이 누락의 물증이다.

  이전 리뷰가 추론으로 남겨 둔 "그럼 `Failed` 반환으로도 안 드러나는가"는 이번에 확증됐다. UE 5.8 자동화 테스트 `System.StateTree.TasksCompletion.IneligibleTaskEnterStateFail`(`StateTreeTaskStateTest.cpp:955-1030`)이 `bConsideredForCompletion=false` 인 태스크의 `EnterState` 실패는 **상태 완료 판정에서 무시된다**("ineligible EnterState failure is ignored")고 명시한다. 즉 세 태스크가 반환하는 `EStateTreeRunStatus::Failed`(`SetQuestTitle.cpp:28`, `SetQuestObjective.cpp:28`, `StartNextQuest.cpp:26`)는 전부 죽은 값이고, 조립 오류를 알릴 수단은 로그밖에 없는데 체인 태스크에만 그 로그가 없다. 실제 증상은 "퀘스트 체인이 다음으로 안 넘어가고 끝남"이며, 체인 종점(빈 지정)과 조립 오류가 겉으로 구분되지 않아 원인 추적이 불가능하다.
- **제안**: 형제와 같은 형식의 `UE_LOG(LogWxQuest, Warning, ...)` 을 추가하고 `WxQuestModule.h` 를 include 한다. 아울러 세 태스크의 doc-comment 에 "완료 판정에서 빠져 있으므로 반환값은 무시되고 진단은 로그가 유일하다"를 명시해 다음 사람이 `Failed` 를 신호로 오해하지 않게 한다.
- **확신도**: 높음

### 5. 🟡 퀘스트 체인 전환마다 게임 스레드 블로킹 로드
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:131-134`
- **범주**: 성능/안전
- **문제**: `HandleDeferredActivateQuest` 가 `LoadSynchronous()` 로 다음 퀘스트 에셋을 받는다. 소프트 참조를 쓴 이유는 "타이머 대기 중 GC 로 로드가 풀릴 수 있어서"(`:47`)인데, 정작 실행 시점 로드가 동기라 체인이 넘어갈 때마다 게임 스레드가 멈춘다. 퀘스트 ST 에셋이 링크드 트리·크로스모듈 노드가 참조하는 에셋까지 물고 있으면 히치가 눈에 띈다. 로드 실패(에셋 삭제·이름 변경) 시에도 `ActivateQuest` 가 조용히 노옵으로 빠져(`:20-23`) 이전 퀘스트가 끝난 채 저널만 빈 상태로 남는다.
- **제안**: `RequestActivateQuest` 예약 시점에 `FStreamableManager::RequestAsyncLoad` 로 예열하고 완료 콜백에서 `ActivateQuest` 를 부른다(예약 취소는 발견 3 의 멤버와 같은 자리에서 처리된다). 로드 실패는 `LogWxQuest` 경고를 남긴다.
- **확신도**: 중간

### 6. 🟢 런타임 생성한 러너를 컴포넌트 파괴 시 정리하지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:106-121`
- **범주**: 버그/정확성
- **문제**: `BeginPlay` 가 `UStateTreeComponent` 를 **GameState 를 Outer 로** 만들어 등록하는데(`:117-120`), 이 파일에는 `EndPlay`·`OnUnregister` 가 없다. 러너의 Outer 가 퀘스트 컴포넌트가 아니므로, 퀘스트 컴포넌트만 파괴되면 러너는 등록·틱 상태 그대로 남아 퀘스트 StateTree 를 계속 돌린다 — 저널을 갱신할 상대는 사라졌는데 월드 부수효과만 계속 나는 상태다. 재주입 시에는 `NewObject` 가 고정 이름 `TEXT("QuestStateTree")` 로 두 번째를 만들려다 살아 있는 기존 오브젝트와 충돌한다(`:117`).

  컴포넌트 단독 파괴 경로는 실재한다 — GameFeature 비활성화 시 `ContextHandles.Remove` 가 `TArray<TSharedPtr<FComponentRequestHandle>>` 를 놓아 주입 컴포넌트를 걷어간다(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h:58-62`, `.cpp:67-78`). 다만 현재 그 시점은 사실상 월드 종료와 묶여 있어 잠복 상태다. 런타임 Experience 교체나 GameFeature 개별 토글이 생기는 순간 드러난다.
- **제안**: `EndPlay`(또는 `OnUnregister`)를 오버라이드해 `OnStateTreeRunStatusChanged` 구독을 끊고 `QuestStateTree->DestroyComponent()` 후 `nullptr` 로 비운다. 발견 3 의 대기 타이머 핸들도 같은 자리에서 지운다.
- **확신도**: 중간 (현재 실행 경로에선 재현되지 않는 잠복 결함)

### 7. 🟢 `HasActiveQuest()` 의 실제 의미는 "제목이 등록됐는가"이며, 정리 수렴도 거기에 묶여 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:51-57`, `:59-68`, `:84-87`, `:136-141`
- **범주**: 설계/구조
- **문제**: `bHasActiveQuest` 를 세우는 곳은 `SetQuestTitle`(`:55`) 하나뿐이다. `ActivateQuest` 도 `AddObjective` 도 이 플래그를 건드리지 않는데, `ClearJournal` 은 `!bHasActiveQuest` 면 조기 반환한다(`:138-141`). 결과가 둘이다.
  1. 이름과 의미가 어긋난다 — 러너는 돌지만 제목 태스크가 아직 진입하지 않은 구간, 또는 `SetQuestTitle` 을 배치하지 않은 퀘스트는 "활성 퀘스트 없음"을 보고한다. 현재 유일한 소비자가 HUD 게이트라 드러나지 않지만(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:50`), 이름만 보고 "퀘스트 진행 중 판정"으로 재사용하는 두 번째 소비자가 생기면 어긋난다.
  2. "저널 정리는 러너의 상태 변경 한 곳으로 수렴한다"(`WxQuestComponent.h:37`)는 규약이 제목 없는 퀘스트에서는 성립하지 않는다 — `ClearJournal` 이 조기 반환하므로 목표 정리가 각 목표 태스크의 `ExitState` 에만 의존하게 된다.
- **제안**: 표시용 플래그가 의도라면 이름을 의미에 맞게(`HasJournalEntry` 등) 바꾼다. 이름을 유지한다면 `AddObjective` 에서도 플래그를 세우거나, `ClearJournal` 의 조기 반환 조건을 "플래그도 없고 목표도 비었을 때"로 넓혀 수렴 규약을 무조건 성립시킨다.
- **확신도**: 중간 (표시 전용 플래그라면 의도된 설계일 수 있음)

### 8. 🟢 저널이 복제되지 않아 리모트 클라이언트 HUD 는 영구히 비어 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:91-102`, `Private/Quest/WxQuestComponent.cpp:110-114`
- **범주**: 설계/구조
- **문제**: `QuestTitle`·`Objectives`·`bHasActiveQuest` 는 복제 지정이 없고 `GetLifetimeReplicatedProps` 도 없다. 러너가 권위에만 생성되므로(`:110-121`) 리모트 클라이언트의 컴포넌트 사본은 저널이 영원히 빈 채로 남는다. 소비자 VM 은 로컬 컴포넌트에서 값을 pull 하므로(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:44-52`) 클라이언트 HUD 에는 퀘스트가 표시되지 않는다. 클래스 주석(`:36`)이 "싱글/리슨 호스트" 전제를 명시하고 있어 의도된 v1 범위로 읽히나, 멀티플레이 마일스톤에서 반드시 되돌아와야 할 지점이므로 목록에 남긴다.
- **제안**: 멀티플레이를 다룰 때 저널 3개 필드를 `ReplicatedUsing` 으로 승격하고 `OnRep` 에서 `OnJournalChanged` 를 발화시킨다. 핸들 기반 목표 목록은 그대로 복제 가능하다.
- **확신도**: 낮음 (문서화된 의도적 범위 제한)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, 모듈 밖 대조용 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`
- **규칙 대조 결과(위반 없음)**: 모듈 의존은 `WxCore` + 엔진 플러그인뿐이고 모듈 밖 유일한 include 인 `WxLocatorUtils.h` 도 `WxCore` 소속이다(`Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`). 전 소스와 `Build.cs` 첫 줄 저작권 표기 존재, 람다 0건, `FORCEINLINE` 0건, `BlueprintCallable` 은 BP Function Library 인 `UWxQuestLibrary` 한 곳뿐(`WxQuestLibrary.h:24`), 델리게이트 콜백 2종 모두 `Handle` 접두(`WxQuestComponent.h:84,87`), `BeginPlay` 의 `Super::` 호출 확인(`WxQuestComponent.cpp:108`). 4개 태스크 헤더의 `GetInstanceDataType()` 인라인 정의는 코딩 규칙 6 이 명문화한 예외이며 요구되는 사유 주석도 각 헤더에 달려 있다(예: `WxStateTreeTask_SetQuestTitle.h:12`).
- **미검토 / 한계**: 퀘스트 `UWxQuestStateTree` 에셋의 실제 저작 내용(상태 구조·태스크 배치)은 BP/에셋 영역이라 보지 않았으므로, 발견 1·2·3 의 "잘못된 조립"·"같은 프레임 중복 예약"이 현재 에셋에서 재현되는지는 확인하지 못했다. 아래 셋은 검토했으나 항목으로 세우지 않았다 — (a) `WxQuest.Build.cs:16` 의 `GameplayTags` 는 모듈 안에서 사용 0건인 죽은 의존이지만 무게가 없다, (b) `WxStateTreeTask_WaitMoveToTarget.cpp:49` 의 `FVector::Dist` 3D 판정은 로케이터 대상이 지면 액터가 아닐 때(공중 마커 등) 반경을 부풀리지만 의도된 단순화로 보인다, (c) 매 틱 `SyncFind`·`GetPlayerController` 는 헤더(`:35`)에 근거가 적혀 있고 활성 퀘스트가 1개뿐이라 비용이 무시된다. 네 태스크 각각이 반복하는 "오너에서 퀘스트 컴포넌트 찾기" 2줄은 중복이지만, 호출부 인라인을 선호하는 프로젝트 방침에 부합하므로 지적하지 않는다.

---
*문서 기준 커밋 `e54feda9` · 리뷰일 2026-08-27 · 소스 15파일 — `/module-review`로 갱신*
