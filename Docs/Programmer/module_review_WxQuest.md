# WxQuest — 코드 리뷰

> 러너 위임·저널·수주 경로가 한 컴포넌트에 얇게 정리된 소형 모듈이고, 프로젝트 코딩·모듈 규칙 위반은 없다. 다만 "상태 완료는 대기 태스크가 낸다"는 핵심 계약이 C++ 이 아니라 에셋 체크박스에 기대고 있고, 런타임 생성 러너에 해제 경로가 없다. 커버리지: 소스 15파일 전부를 읽고 `WxQuestComponent.cpp` 와 ST 태스크 4개 cpp 를 정독했으며, 완료 판정·러너 수명 관련 주장은 UE 5.8 엔진 소스(`StateTreeTaskBase.h`·`StateTreeTasksStatus.h`·`StateTreeState.h`·`StateTreeComponent.cpp`)와 프로젝트 측 `WxGameFeatureAction_AddComponents.cpp` 로 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 런타임에 붙인 러너에 해제 경로가 없다 — 컴포넌트만 회수되면 GameState 에 남아 계속 실행된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:106-121`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:79`
- **범주**: 설계/구조
- **문제**: `BeginPlay` 가 `NewObject<UStateTreeComponent>(Owner, TEXT("QuestStateTree"))` 로 러너를 만들어 **오너(GameState)** 에 `RegisterComponent` 하는데(`cpp:117-119`), 오버라이드가 `BeginPlay` 하나뿐이라(`h:79`) 짝이 되는 해제가 없다. 러너의 오너는 GameState 이지 이 컴포넌트가 아니므로, GameState 는 살아 있고 `UWxQuestComponent` 만 사라지는 경로에서 러너가 남아 퀘스트 ST 를 계속 돌린다. 부착이 Experience 의 컴포넌트 주입(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:152` 의 `AddComponentRequest` 핸들)이라, 핸들이 풀리면 액터는 살아 있는 채 컴포넌트 인스턴스만 파괴되는 바로 그 조합이 열려 있다. 그때 저널 태스크는 컴포넌트를 못 찾아 경고만 남기지만(`WxStateTreeTask_SetQuestTitle.cpp:21`, `WxStateTreeTask_SetQuestObjective.cpp:21`) 퀘스트 ST 안의 크로스모듈 태스크(스포너 기동·보상)는 그대로 나간다.
  이름이 `TEXT("QuestStateTree")` 로 고정이라 재주입 시엔 `NewObject` 가 동명 오브젝트를 그 자리에서 파괴·재구성한다 — `StopLogic` 을 거치지 않은 채 끌려 내려간다.
- **제안**: `EndPlay`(또는 `OnUnregister`)를 오버라이드해 `QuestStateTree->StopLogic()` → `DestroyComponent()` → 포인터 정리. 세 줄이면 유령 실행과 in-place 덮어쓰기가 함께 사라진다.
- **확신도**: 높음(메커니즘 확정. 현재 콘텐츠에서 Experience 교체는 대개 맵 이동과 함께 와 GameState 가 통째로 정리되므로 실측 빈도는 낮다)

### 2. 🟡 "상태 완료는 대기 태스크가 낸다"는 계약을 코드가 보증하지 않는다 — 엔진 기본값이 반대다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:10-13`(생성자)·`:29`(반환), `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:10-13`·`:29`, 계약 서술 `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h:30`, `Plugins/WxQuest/README.md:34`
- **범주**: 설계/구조
- **문제**: 헤더와 README 는 "즉시 끝나는 태스크는 진입 즉시 `Succeeded`, 상태의 실제 완료는 짝이 되는 대기 태스크가 낸다"고 규정한다. 이 규정은 그 상태의 `TasksCompletion` 이 `All` 일 때만 성립하는데, 엔진의 상태 기본값은 `EStateTreeTaskCompletionType::Any` 이고(`StateTreeState.h:428`) `Any` 에서는 판정 대상 태스크 중 **하나라도** Succeeded 면 그 자리에서 상태가 완료된다(`StateTreeTasksStatus.h:100-104`). 두 태스크 모두 `bConsideredForCompletion` 을 건드리지 않아 기본값 `true`(`StateTreeTaskBase.h:34`)로 판정에 참여한다.
  즉 디자이너가 새 스텝 상태를 기본값으로 만들고 `[SetQuestObjective, WaitMoveToTarget]` 를 얹으면, 목표를 등록한 그 프레임에 상태가 `Succeeded` 로 끝나고 `WaitMoveToTarget` 은 한 번도 판정되지 않는다 — 퀘스트가 스텝을 통째로 건너뛴다. 기존 에셋이 `All` 로 지정돼 있어 지금 안 터지는 것뿐이고, 실패는 조용하다(로그·검증 없음). 프로젝트의 다른 ST 태스크 중 `bConsideredForCompletion` 을 코드로 못박은 곳은 현재 한 군데도 없어, 같은 함정이 도메인 공통으로 열려 있다.
- **제안**: 두 갈래 중 하나. ① 즉시 완료 태스크에 `#if WITH_EDITORONLY_DATA bConsideredForCompletion = false; #endif` 를 되살려 `Any`/`All` 어느 쪽에서도 대기 태스크만 판정하게 한다(단, 대기 태스크가 없는 상태는 판정 대상이 0이 되는 README:34 의 함정과 짝을 이루므로 그 규약을 함께 손봐야 한다). ② 코드를 그대로 두려면 최소한 헤더·README 에 "이 계약은 상태의 `TasksCompletion=All` 을 전제한다"를 명시하고, 가능하면 에셋 검증(`IsDataValid`)으로 기본값 `Any` 상태를 걸러낸다.
- **확신도**: 중간(엔진 동작은 소스로 확정. 현재 에셋이 전부 `All` 이라 즉시 발현하지는 않으므로 "지금 버그"가 아니라 "기본값이 반대인 함정"이다)

### 3. 🟡 즉시 완료 태스크 3종이 `bShouldStateChangeOnReselect` 기본값을 그대로 둬 유지 재선택마다 부수효과를 재실행한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:10-13`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:10-13`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:10-13`
- **범주**: 버그/정확성
- **문제**: 세 생성자 모두 `bShouldCallTick = false` 만 세팅한다. `bShouldStateChangeOnReselect` 는 기본값 `true`(`StateTreeTaskBase.h:24`)라, 부모 상태가 유지된 채 형제 상태로 전이할 때마다 그 부모의 태스크가 `ExitState → EnterState` 로 다시 불린다. 결과:
  - `SetQuestTitle` 은 `EnterState` 마다 `Objectives.Reset()` 을 돈다(`WxQuestComponent.cpp:54`). 상태 내 태스크 순서상 목표 태스크가 제목 태스크보다 **앞에** 놓이면 그 스텝의 목표가 등록 직후 지워진다. 순서가 반대면 결과는 같아지지만, 저널 브로드캐스트가 전이 1회당 3번 튄다 — 구독자(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:55-66`)가 브로드캐스트마다 목표 수만큼 `NewObject` 로 뷰모델을 새로 만들므로 전이마다 불필요한 UObject 할당이 반복된다.
  - `StartNextQuest` 는 재진입 때마다 `RequestActivateQuest` 를 한 번 더 걸어(발견 4와 결합) 다음 퀘스트를 두 번 시작한다.
  같은 코드베이스의 다른 ST 태스크 10곳(`WxDialogue/WxStateTreeTask_PlayDialogue.cpp:19`, `WxUI/WxStateTreeTask_MarkIndicator.cpp:101`, `WxUI/WxStateTreeTask_PrintSubtitle.cpp:13`, `WxWorld` 6곳, 그리고 같은 모듈의 `WxStateTreeTask_WaitMoveToTarget.cpp:16`)은 전부 `false` 로 못박아 뒀다 — WxQuest 의 이 세 개만 빠져 있다.
- **제안**: 세 생성자에 `bShouldStateChangeOnReselect = false` 를 추가한다. 저널 등록·체인 예약은 상태에 처음 들어갈 때 한 번만 유효한 부수효과이므로 재선택 재진입이 필요 없다.
- **확신도**: 중간(엔진 재진입 조건은 소스로 확정. 실제 퀘스트 에셋의 상태 계층에 유지 재선택 전이가 있는지는 에셋을 열지 않아 미확인)

### 4. 🟡 `RequestActivateQuest` 가 예약 핸들을 보관하지 않아 요청끼리 서로를 덮는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:40-49`
- **범주**: 버그/정확성
- **문제**: 호출마다 별개의 next-tick 타이머를 걸고 `FTimerHandle` 을 버린다(`:48`). 같은 프레임에 두 번 예약되면 둘 다 발화해 `ActivateQuest` 가 두 번 돌고, 뒤엣것이 앞엣것을 **정지 후 재시작**시킨다 — 먼저 시작된 퀘스트의 첫 프레임 진행(제목·목표 등록)이 버려진다. 예약이 살아 있는 사이 트리거 볼륨이 `UWxQuestLibrary::StartQuest` 로 즉시 활성화해도 마찬가지로 다음 틱에 덮인다. "동시 1개, 새 시작이 교체"라는 모델에서 승자가 요청 순서가 아니라 타이머 발화 순서로 정해진다.
  같은 함수의 `GetWorld()->GetTimerManager()`(`:48`)도 널 검사가 없다 — 컴포넌트 정리 중 호출되면 역참조한다.
- **제안**: `FTimerHandle` 과 예약 대상을 멤버로 두고, 새 요청이 기존 예약을 `ClearTimer` 로 덮게 한다. `GetWorld()` 는 널 가드를 붙인다.
- **확신도**: 높음(메커니즘 확정. 현재 콘텐츠에서 같은 프레임 중복 예약이 실제로 나는지는 미확인)

### 5. 🟡 빈 로케이터의 `WaitMoveToTarget` 은 경고 한 줄 뒤 영구 교착이다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:19-29`, `:31-55`
- **범주**: 버그/정확성
- **문제**: 진입 시 빈 로케이터를 경고로 알리지만(`:23-26`) 그대로 `Running` 을 반환하고, `Tick` 은 대상이 해석되지 않으면 계속 `Running` 이다(`:48-54`). 빈 로케이터는 스트리밍 아웃과 달리 나중에 해석될 여지가 없으므로 회복 불가능한 교착이다 — 퀘스트가 그 스텝에 영원히 멈추고, 활성 퀘스트가 1개뿐이라 다른 퀘스트로 넘어갈 수도 없다. 경고는 진입 시 한 번뿐이라 나중에 로그를 봐도 눈에 띄지 않는다.
- **제안**: 빈 로케이터는 경고 후 `Failed` 를 반환해 명시적으로 실패시킨다. 비어 있지 않은데 해석만 안 되는 스트리밍 아웃은 지금처럼 대기가 맞다.
- **확신도**: 중간(교착 자체는 확정. `Failed` 로 바꿀지는 퀘스트 트리에 실패 전이가 있는지에 달린 콘텐츠 정책 문제)

### 6. 🟡 같은 퀘스트를 다시 활성화하면 진행이 조용히 초기화된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:18-38`
- **범주**: 버그/정확성
- **문제**: `ActivateQuest` 는 인자가 지금 돌고 있는 바로 그 에셋이어도 무조건 정지·교체·재시작을 수행한다(`:35-37`). 진행 상태·목표·인스턴스 데이터가 전부 버려지고 트리가 루트부터 다시 시작되며, 저널도 한 번 비워졌다 다시 채워진다. 반환값도 로그도 없다. 수주 경로가 레벨 배치 볼륨의 오버랩이라 플레이어가 진행 중 그 볼륨으로 되돌아오면 그대로 발현하고, 이미 지나온 스텝의 스포너·보상 구간이 다시 돈다. "다른 퀘스트로의 교체"는 설계 의도지만 "같은 퀘스트로의 교체까지 리셋"이어야 할 근거는 코드·문서 어디에도 없다.
- **제안**: 진입부에 "요청 에셋 == 현재 실행 중 에셋 && 러너가 Running 이면 노옵" 가드를 둔다. 진입점이 라이브러리 하나로 모여 있으니 코드에서 막는 편이 누락이 없다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 볼륨 BP 이벤트 그래프에 자체 1회성 가드가 있는지는 이번 범위 밖이라 확인하지 못했다)

### 7. 🟢 `StartNextQuest` 만 컴포넌트 부재 시 아무 흔적을 남기지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:19-22`
- **범주**: 버그/정확성
- **문제**: 다른 두 저널 태스크는 오너 이름까지 담아 경고를 남기는데(`WxStateTreeTask_SetQuestTitle.cpp:21-22`, `WxStateTreeTask_SetQuestObjective.cpp:21-22`) 이 태스크만 조용히 `Failed` 를 돌려준다. `Failed` 는 그 상태를 실패시켜 퀘스트 체인을 끊는데 원인은 어디에도 찍히지 않는다. 세 태스크가 같은 "오너에서 컴포넌트를 찾고 없으면 Failed" 전문을 복제하고 있으므로 형태를 맞추는 비용도 없다.
- **제안**: 앞의 둘과 같은 형태로 `UE_LOG(LogWxQuest, Warning, ...)` 한 줄 추가.
- **확신도**: 높음

### 8. 🟢 `Build.cs` 에 쓰이지 않는 의존이 남아 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`(`GameplayTags`), `:20`(`WxCore`)
- **범주**: 중복/복잡도
- **문제**: 모듈 소스 전체에서 `GameplayTag` 식별자 사용이 0건이고, `WxCore` 헤더 include 도 0건이다(`Plugins/WxQuest/Source/WxQuest` 전수 검색). README(`:18-19`)는 "주요 의존: WxCore" 라고 적었지만 실제로는 코드 결합이 없다.
- **제안**: `GameplayTags` 는 제거한다. `WxCore` 는 도메인 플러그인 공통 컨벤션이라 유지할 수 있으나, 유지한다면 README 의 "주요 의존" 서술을 실태에 맞게 고친다.
- **확신도**: 높음

### 9. 🟢 퀘스트 체인이 게임 스레드 동기 로드로 이어진다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:131-134`
- **범주**: 성능/안전
- **문제**: `HandleDeferredActivateQuest` 가 `QuestAsset.LoadSynchronous()`(`:133`)로 다음 퀘스트 에셋을 게임 스레드에서 통째로 끌어온다. 소프트 참조를 쓴 이유(GC 로 로드가 풀릴 수 있음)는 주석에 적혀 있으나, 로드 방식까지 동기일 필요는 없다. 체인 전환이 곧 스텝 완료 직후 연출 구간과 겹치므로 히치가 눈에 띄기 쉬운 자리다.
- **제안**: `FStreamableManager::RequestAsyncLoad` 로 비동기 로드 후 완료 콜백에서 `ActivateQuest` 를 호출한다. 지금도 이미 한 틱 지연을 감수하는 구조라 비동기 전환의 추가 비용이 없다.
- **확신도**: 중간(에셋 크기에 따라 체감 히치가 무시할 수준일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md` — 교차 확인용으로 엔진(UE 5.8) `StateTreeTaskBase.h`·`StateTreeTasksStatus.h`·`StateTreeState.h`·`StateTreeComponent.cpp`, 프로젝트 측 `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`·`Source/WxGame/MVVM/WxViewModel_Quest.cpp`, 대조용으로 `WxDialogue`·`WxUI`·`WxWorld` 의 ST 태스크 생성자 플래그 설정
- **미검토 / 한계**: 퀘스트 StateTree 에셋(`Content/Quest/*`) 내부 조립(상태 계층·전이·상태별 `TasksCompletion`·노드 순서)은 이번에 열지 않았다 — 발견 2·3·5 의 실제 발현 여부는 그 조립에 달려 있다. 트리거 볼륨 BP 의 이벤트 그래프는 범위 밖이라 발견 6 의 재트리거 가드 존재 여부는 미확인이다. 퀘스트 체인(`StartNextQuest`)을 쓰는 에셋이 있는지도 확인하지 않아 그 경로의 실동작은 미검증이다. 멀티플레이 관련 두 가지(저널이 복제되지 않아 전용 서버의 원격 클라이언트에서는 비는 점, `WaitMoveToTarget` 이 0번 컨트롤러만 보는 점)는 헤더(`WxQuestComponent.h:36`, `WxStateTreeTask_WaitMoveToTarget.h:31`)와 README 가 v1 싱글/리슨 호스트 전제로 명시한 의도된 범위라 발견으로 올리지 않았다. 규칙 준수는 별도로 훑었고(첫 줄 저작권 15/15, `Wx` prefix, 델리게이트 콜백 `Handle` prefix 2/2, `BlueprintCallable` 은 BP Function Library 1건뿐, `BeginPlay` 의 `Super::` 호출, `WxCore` 외 Wx 플러그인 미참조, 람다 없음) 위반은 없다 — 헤더의 `GetInstanceDataType()` 인라인 정의는 규칙 6 에 문언상 걸리나 `2026-07-31` 워크로그에서 전 도메인 공통 예외로 결정되고 각 헤더에 근거 주석(`WxStateTreeTask_SetQuestObjective.h:12` 등)이 달려 있어 제외했다.

---
*문서 기준 커밋 `e9440f73` · 리뷰일 2026-08-15 · 소스 15파일 — `/module-review`로 갱신*
