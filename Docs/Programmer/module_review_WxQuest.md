# WxQuest — 코드 리뷰

> 15파일 규모의 작고 응집도 높은 모듈이다. 권위 경계·저널 수명·러너 재진입 회피처럼 틀리기 쉬운 지점이 헤더 주석과 README 에 정확히 문서화돼 있어 전반적으로 건강하고, 코딩·모듈 규칙 위반은 한 건도 없다. 남은 지적은 거의 전부 "엔진이 조용히 거절하거나 무시하는 실패 경로를 코드가 성공으로 취급한다" 계열에 몰려 있다. 커버리지: 모듈의 15개 소스를 전부 읽었고, 판정 근거로 UE 5.8 의 `UStateTreeComponent`·`FStateTreeExecutionContext::Stop`·`StateTreeTaskBase`·`StateTreeCompiler`·`StateTreeTaskStateTest` 구현과 `Source/WxGame` 의 소비처(저널 뷰모델, 컴포넌트 주입 액션)까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 즉발 태스크 3종이 내는 `Failed` 를 엔진이 무시한다 — 조립 오류를 드러내는 장치가 사실상 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:15,28`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:15,28`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:15,26`
- **범주**: 버그/정확성
- **문제**: 세 태스크 모두 생성자에서 `bConsideredForCompletion = false` 를 켜 두고, 퀘스트 컴포넌트를 못 찾으면 `EnterState` 에서 `EStateTreeRunStatus::Failed` 를 반환한다. 그런데 UE 5.8 은 완료 판정에서 제외된 태스크의 `EnterState` 실패를 **의도적으로 버린다** — 컴파일러가 `bConsideredForCompletion` 을 `CompactState.CompletionTasksMask` 로 굽고(`StateTreeCompiler.cpp:372-407,1365-1385`), 실행부는 마스크 밖 태스크의 실패를 상태 실패로 승격하지 않는다. 엔진 테스트가 이 계약을 못 박고 있다: `StateTreeTaskStateTest.cpp:955-1030` 의 `FStateTreeTest_TasksCompletion_IneligibleTaskEnterStateFail` — "An ineligible task returning Failed from EnterState must not prevent the state from entering". 즉 이 `Failed` 의 유일한 실효는 "그 태스크가 이후 Tick 되지 않는다"인데, 셋 다 `bShouldCallTick = false` 라 그마저도 의미가 없다. 실제 결과는 **상태가 그대로 진입해 퀘스트가 계속 돌고, 저널만 영원히 비어 있는 것**이다. 이 경로는 도달 가능하다 — 태스크들이 스키마 제한 없이 `FStateTreeTaskCommonBase` 를 상속해 `UStateTreeComponentSchema` 기반 트리(AI·기믹 등) 어디에나 노출되므로, 기획자가 퀘스트 러너가 아닌 트리에 이 노드를 얹으면 그대로 물린다. README 와 각 헤더 주석이 "없으면 잘못된 조립이므로 `Failed` 를 낸다"고 적어 둔 계약이 코드/엔진 실제와 어긋난다.
- **제안**: 실패 신호를 `Failed` 반환에 의존하지 않는다. (a) 세 태스크 모두 `WxQuestModule.h` 의 `LogWxQuest` Error 로 승격해 조립 오류를 로그로 못 박고, (b) 정말 퀘스트를 끊고 싶다면 `Context.FinishTask(...)`/트리 정지 같은 완료 마스크와 무관한 경로를 쓰거나 컴포넌트 쪽에서 러너를 정지시킨다. 최소 조치로도 헤더·README 의 "Failed 를 낸다 → 퀘스트가 실패한다"는 서술은 정정해야 다음 세션이 같은 오해를 반복하지 않는다.
- **확신도**: 높음

### 2. 🟡 `ActivateQuest` 의 세 엔진 호출이 전부 거절 가능한데 반환을 아무도 검사하지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:33-37` (외부 진입점 `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`)
- **범주**: 설계/구조
- **문제**: `StopLogic` → `SetStateTreeReference` → `StartLogic` 은 셋 다 조건부로 아무 일도 하지 않고 반환하는 함수인데 코드는 세 호출이 항상 관철된다고 가정한다. 러너 실행 콜스택 안에서 호출되면 다음 연쇄가 확정적으로 난다 — `FStateTreeExecutionContext::Stop` 은 재진입 호출을 프레임 끝으로 미루고 **`EStateTreeRunStatus::Running` 을 그대로 반환**하므로(`StateTreeExecutionContext.cpp` 의 `Exec.CurrentPhase != Unset` 분기), `UStateTreeComponent::StopTree` 는 상태 변화가 없다고 보아 `OnStateTreeRunStatusChanged` 를 쏘지 않는다(저널 정리 누락) → 이어지는 `SetStateTreeReference` 는 실행 상태가 여전히 Running 이라 경고만 남기고 교체를 거부(`StateTreeComponent.cpp:491-503`) → `StartLogic` → `StartTree` 는 `CurrentlyRunningExecContext` 가 살아 있어 "Reentrant call is not allowed" 에러로 반환(`StateTreeComponent.cpp:181-185`). 순 결과는 **요청한 퀘스트가 조용히 유실되고, 진행 중이던 퀘스트만 프레임 끝에 정지되는 것**이며 흔적은 `LogStateTree` 두 줄뿐이다. 규약(`Public/Quest/WxQuestComponent.h:54`)이 "ST 실행 콜스택 밖에서만 호출"인데 그 유일한 외부 진입점은 `BlueprintCallable` 이라 준수 책임이 BP 작성자에게 전가돼 있고, 하필 `OnJournalChanged` 는 `BlueprintAssignable` 이면서 `EnterState` 안에서 broadcast 되므로(`WxQuestComponent.cpp:56` ← `WxStateTreeTask_SetQuestTitle.cpp:32`) 이 델리게이트를 받아 `StartQuest` 를 부르는 BP 하나면 규약이 깨진다.
- **제안**: `ActivateQuest` 를 private 으로 내리고 외부 진입점을 `RequestActivateQuest`(다음 틱) 하나로 일원화해 규약 자체를 없앤다 — 체인 경로가 이미 그 1틱 지연을 감수하고 있다. 더불어 `SetStateTreeReference` 직후 러너의 실제 참조가 요청한 에셋인지 확인하고, 어긋나면 `StartLogic` 을 건너뛴 뒤 `LogWxQuest` Error 를 남긴다.
- **확신도**: 높음

### 3. 🟡 시작 실패를 확인하지 않아 "퀘스트도 저널도 없는" 상태로 조용히 빠질 수 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:18-38`
- **범주**: 버그/정확성
- **문제**: 순서가 정지 → 교체 → 시작이라, 새 에셋이 시작 가능한지 확인하기 전에 진행 중 퀘스트를 먼저 버린다. `UStateTreeComponent::StartTree` 는 `HasValidStateTreeReference()` 가 실패하면(`StateTreeComponent.cpp:449-468`: 에셋 미설정 / `IsReadyToRun()` false / 스키마가 `UStateTreeComponentSchema` 파생이 아님) `bIsRunning = false; DisableTick(); return;` 으로 **아무 broadcast 없이** 끝난다. `UWxQuestStateTree` 는 표식용 빈 서브클래스라 스키마를 강제하지 않으므로, 스키마를 잘못 고른 퀘스트 에셋 하나로 이 분기에 들어간다. 그때 WxQuest 쪽에는 신호가 전혀 없다 — 이전 퀘스트는 이미 정지·정리됐고, 새 퀘스트는 시작되지 않았으며, `bHasActiveQuest` 는 false 이고 러너는 못 쓰는 참조를 쥔 채 남는다. 플레이어는 퀘스트 없는 상태로 계속 플레이하고 원인은 `LogStateTree` 로만 남는다.
- **제안**: `StartLogic()` 직후 `QuestStateTree->IsRunning()`(또는 `GetStateTreeRunStatus()`) 을 확인해 실패면 `LogWxQuest` Error 로 에셋 이름을 지목한다. 나아가 `SetStateTreeReference` 전에 검증을 끝내 진행 중 퀘스트를 버리지 않는 순서로 바꾸는 편이 안전하다.
- **확신도**: 중간

### 4. 🟡 예약된 다음 퀘스트 활성화가 취소·병합되지 않아 그 사이 수주를 덮어쓴다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:40-49`
- **범주**: 버그/정확성
- **문제**: `RequestActivateQuest` 는 `SetTimerForNextTick` 을 호출만 하고 `FTimerHandle` 을 보관하지 않아 예약을 취소하거나 중복을 병합할 수단이 없다. 결과는 두 가지다. (a) 체인 예약이 걸린 프레임에 레벨 트리거가 `UWxQuestLibrary::StartQuest` 로 다른 퀘스트를 즉시 수주시키면, 다음 틱에 뒤늦게 도는 체인 예약이 그 퀘스트를 정지·교체해 **방금 받은 퀘스트가 시작하자마자 사라진다**. 오픈월드에서 트리거 볼륨과 체인 종료가 겹치는 건 드문 조합이 아니다. (b) 같은 프레임에 예약이 둘 이상 쌓이면(체인 태스크가 있는 상태가 Sustained 재선택되는 경우 — `FWxStateTreeTask_StartNextQuest` 는 `bShouldStateChangeOnReselect` 를 끄지 않아 재진입한다) 첫 퀘스트가 진입 상태의 월드 부수효과(스폰·보상)만 남기고 즉시 교체된다.
- **제안**: 예약 핸들과 예약된 에셋을 멤버로 보관하고, 새 예약·즉시 활성화 시 기존 예약을 `ClearTimer` 로 무효화한다(마지막 요청 우선). 발견 6 의 비동기 로드로 옮길 경우 스트리밍 핸들이 그 역할을 겸할 수 있다.
- **확신도**: 중간

### 5. 🟡 런타임 생성한 러너 컴포넌트에 대칭 정리가 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:106-121` (오버라이드 목록 `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:79`)
- **범주**: 설계/구조
- **문제**: `BeginPlay` 가 오너에 `UStateTreeComponent` 를 만들어 `RegisterComponent` 하고 델리게이트까지 걸지만 `EndPlay`·`OnUnregister` 오버라이드가 없어 소유권이 한 방향으로만 성립한다. 본 컴포넌트는 코드가 아니라 Experience 주입으로 붙고, 프로젝트의 주입 액션은 GameFeature 비활성 시 `ContextHandles.Remove(Context)` 로 `FComponentRequestHandle` 을 놓아(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:78`, 핸들 소유는 `WxGameFeatureAction_AddComponents.h:61`) 주입 컴포넌트를 파괴한다. 반면 러너는 `UGameFrameworkComponentManager` 가 추적하지 않으므로 GameState 에 남아 계속 틱하며 퀘스트 태스크(스폰·보상 등 월드 부수효과)를 구동한다. 재주입 시엔 고정 이름 `TEXT("QuestStateTree")` 로 같은 Outer 에 `NewObject` 하므로 살아 있는 기존 러너와 이름이 충돌한다. 현재는 비활성이 월드 티어다운에서만 일어나 실피해가 없는 잠복 결함이지만, 인게임 Experience 교체가 생기는 순간 드러난다.
- **제안**: `EndPlay`(또는 `OnUnregister`) 에서 `OnStateTreeRunStatusChanged` 바인딩을 풀고 `StopLogic` 후 러너를 `DestroyComponent` 한다.
- **확신도**: 중간

### 6. 🟢 퀘스트 체인 전환 때 게임 스레드에서 에셋을 동기 로드한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:131-134`
- **범주**: 성능/안전
- **문제**: `HandleDeferredActivateQuest` 가 `LoadSynchronous()` 로 다음 퀘스트 에셋을 게임 스레드에서 끌어온다. 참조를 `TSoftObjectPtr` 로 둔 목적 자체가 선로드 회피이므로 콜드 로드가 기본값이고, 로드 범위는 ST 에셋 하나가 아니라 그 안 태스크 인스턴스 데이터가 하드 참조하는 것들(크로스모듈 태스크의 액터 클래스·보상 테이블 등)까지다. 시점이 퀘스트 단계 완료 직후라 다른 연출과 겹치기 쉽고, 인게임이라 히치가 그대로 노출된다.
- **제안**: `FStreamableManager::RequestAsyncLoad` 로 바꾸고 핸들을 멤버로 보관해 완료 콜백에서 `ActivateQuest` 를 호출한다(핸들 보관이 곧 GC 방지라 다음 틱 지연의 원래 목적도 함께 만족하고, 발견 4 의 예약 취소 수단도 된다).
- **확신도**: 중간

### 7. 🟢 `StartNextQuest` 만 조립 오류를 진단 없이 삼키고, 헤더 주석은 경고를 남긴다고 잘못 적혀 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:22-27`
- **범주**: 버그/정확성
- **문제**: 컴포넌트를 못 찾으면 로그 없이 `Failed` 만 반환한다(그리고 발견 1 에 따라 그 `Failed` 는 무시된다). 형제 태스크는 같은 상황에서 오너 이름까지 찍은 경고를 남기며(`WxStateTreeTask_SetQuestTitle.cpp:26`, `WxStateTreeTask_SetQuestObjective.cpp:26`), 이 태스크의 헤더 주석(`Public/Quest/WxStateTreeTask_StartNextQuest.h:27`)도 "경고를 남기고 예약하지 않는다"고 적혀 있어 코드와 어긋난다. 이 태스크의 실패는 퀘스트 체인이 끊긴다는 뜻이라 오히려 가장 진단이 필요한 지점이다.
- **제안**: 형제 태스크와 같은 형태의 `UE_LOG(LogWxQuest, ...)` 를 추가한다(`WxQuestModule.h` include 필요).
- **확신도**: 높음

### 8. 🟢 도달 대상이 비면 퀘스트가 경고 한 줄만 남기고 영구 정지한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23-28`
- **범주**: 설계/구조
- **문제**: 빈 로케이터에서 경고를 남기고도 `Running` 을 반환하고, `Tick` 은 `SyncFind` 가 계속 null 이라 영원히 `Running` 을 반환한다(`:48-54`). 이 태스크가 상태 완료를 내는 유일한 축이므로 결과는 되돌릴 수 없는 소프트락이고 다음 체인까지 함께 죽는다. 아이러니하게도 이 태스크는 완료 판정에 포함된 유일한 태스크라 `Failed` 반환이 실제로 먹히는 유일한 지점인데(발견 1 참조) 여기서만 실패를 내지 않는다.
- **제안**: 빈 로케이터는 `EnterState` 에서 `Failed` 를 반환한다 — 러너가 멈추면 `HandleStateTreeRunStatusChanged` 가 저널을 걷어가 조립 오류가 바로 드러난다. 대상이 스트리밍 아웃돼 일시적으로 해석되지 않는 경우는 지금처럼 대기를 유지하면 된다.
- **확신도**: 중간 (헤더 주석이 "경고를 남긴다"를 명시적 선택으로 적어 두어 의도일 수 있음)

### 9. 🟢 `bHasActiveQuest` 가 "제목이 등록됨"을 "퀘스트가 활성"의 대용으로 쓴다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:55`, `:84-87`, `:136-141`
- **범주**: 설계/구조
- **문제**: 이 플래그를 켜는 곳은 `SetQuestTitle` 하나뿐이다. 제목 태스크를 두지 않은 퀘스트는 목표가 실제로 걸려 있어도 `HasActiveQuest()` 가 false 라 HUD 뷰모델이 저널을 숨기고(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:50`), `ClearJournal` 도 이 플래그를 게이트로 조기 반환하므로 종료 시 정리 통지가 나가지 않는다. 러너의 실행 여부라는 진짜 근거를 이미 컴포넌트가 쥐고 있는데 별도 플래그로 근사한 형태다.
- **제안**: 플래그를 없애고 `HasActiveQuest()` 를 `QuestStateTree && QuestStateTree->IsRunning()` 로 파생시킨다. `ClearJournal` 의 조기 반환은 제목·목표가 이미 비었는지로 판정한다.
- **확신도**: 중간

### 10. 🟢 `SetQuestTitle` 이 목표 목록을 통째로 비워, 아직 살아 있는 목표가 복구 불가능하게 사라진다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:51-57`
- **범주**: 버그/정확성
- **문제**: `Objectives.Reset()` 으로 목록을 비우지만 그 목표를 발급한 `FWxStateTreeTask_SetQuestObjective` 인스턴스들은 여전히 자기 상태에 살아 있다. 목표는 `EnterState` 에서만 등록되므로 한 번 지워지면 그 상태가 끝날 때까지 되살아나지 않고, 나중에 도는 `ExitState` 의 `RemoveObjective` 는 없는 핸들을 조용히 무시하도록 돼 있어(`:70-82`) 진단도 남지 않는다. 표준 조립(루트에 제목, 하위 상태에 목표)에서는 EnterState 가 부모→자식 순서라 결과가 수렴하지만, (a) 같은 상태에서 목표 태스크를 제목 태스크보다 위에 배치하거나 (b) 하위 상태에서 제목을 한 번 더 갱신하면 상위 상태가 건 목표가 전멸한다. 둘 다 에디터에서 막히지 않는다.
- **제안**: `SetQuestTitle` 에서 `Objectives.Reset()` 을 빼고 저널 비우기를 `ClearJournal` 한 곳으로 일원화한다. 제목 재설정 시 비우는 것이 의도라면, 남은 핸들이 있는 상태의 Reset 에 경고를 남겨 조립 실수를 드러낸다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 헤더가 "목표는 비움"을 제목 태스크의 계약으로 명시)

### 11. 🟢 사용하지 않는 빌드 의존 `GameplayTags`
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`
- **범주**: 중복/복잡도
- **문제**: 모듈 전체에 `GameplayTag` 사용이 한 건도 없다(전수 grep 0건). 같은 목록의 `WxCore` 도 include 가 없지만 전 플러그인이 일괄로 싣는 프로젝트 관례이므로 별건으로 보지 않는다.
- **제안**: 항목을 지운다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h`, 태스크 헤더 4종, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`
- **참고로 읽은 모듈 밖 코드**: `Source/WxGame/MVVM/WxViewModel_Quest.cpp`(저널 소비처), `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`·`.h`(부착·회수 경로), 엔진 `StateTreeComponent.cpp`·`StateTreeExecutionContext.cpp`·`StateTreeTaskBase.h`·`StateTreeCompiler.cpp`·`StateTreeTaskStateTest.cpp`(발견 1·2·3·5 의 판정 근거)
- **미검토 / 한계**:
  - 규칙 점검 결과 위반 없음 — 저작권 첫 줄(16/16, Build.cs 포함)·`Wx` prefix·`Handle` 콜백 prefix·`Super::BeginPlay` 호출·`BlueprintCallable` 사용처(BP Function Library 한 곳뿐)·람다 부재(0건)·`WxCore` 외 Wx 플러그인 참조 없음 모두 충족. 태스크 헤더의 `GetInstanceDataType()` 인라인 정의 4건은 각 헤더가 코딩 규칙 6 의 예외임을 명시해 두어 발견으로 올리지 않았다(다만 `CLAUDE.md` 규칙 6 에 그 예외가 적혀 있지 않아, 매 리뷰마다 재판정되는 것을 막으려면 규칙 쪽에 한 줄 명문화하는 편이 낫다).
  - 세 즉발 태스크의 `bConsideredForCompletion = false` 를 `WITH_EDITORONLY_DATA` 로 감싼 것은 정상이다 — 컴파일러가 에디터에서 이 값을 `CompletionTasksMask` 로 구워 넣으므로 쿠킹 빌드에서도 동작이 같다(`StateTreeCompiler.cpp:1365-1385` 확인). 발견으로 올리지 않았다.
  - 퀘스트 StateTree 에셋(`/Game/Quest/**`)의 상태 구성·태스크 배치는 BP/에셋 영역이라 범위 밖이다. 특히 "스스로 끝나야 하는 상태에는 대기 태스크를 하나 둔다"는 README 규약과 발견 10 의 태스크 배치 순서 문제는 코드로 준수 여부를 확인할 수 없다.
  - 저널이 복제되지 않아(`Public/Quest/WxQuestComponent.h:95-102` 의 세 필드 전부 비복제 평범 멤버) 데디케이티드 구성의 클라이언트에서는 뷰모델 구독은 성립하는데 통지가 영영 오지 않는다. 헤더·README 가 v1 싱글/리슨 호스트 전제를 명시한 알려진 보류 사항이라 발견으로 올리지 않았고, 멀티 정책을 켤 때 손볼 지점으로만 남긴다(`WxStateTreeTask_WaitMoveToTarget.cpp:41` 의 0번 컨트롤러 전제도 같은 묶음).
  - `FUniversalObjectLocator::SyncFind` 와 `UGameplayStatics::GetPlayerController` 를 캐시 없이 매 틱 도는 비용(`WxStateTreeTask_WaitMoveToTarget.cpp:41-48`)은 동시 활성 대기 태스크가 1개뿐인 구조상 경로 조회 수준으로 판단해 성능 지적에서 제외했다(실측하지 않음).
  - `RequestActivateQuest` 의 무검증 `GetWorld()` 역참조(`WxQuestComponent.cpp:48`)는 러너가 살아 있는 동안 오너 월드가 항상 유효하므로 발견으로 올리지 않았다.

---
*문서 기준 커밋 `6b77c352` · 리뷰일 2026-08-21 · 소스 15파일 — `/module-review`로 갱신*
