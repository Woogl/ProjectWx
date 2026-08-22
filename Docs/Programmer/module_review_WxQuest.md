# WxQuest — 코드 리뷰

> 소규모(15파일)·단일 책임 모듈로 건강하다. 권위 모델(러너는 서버에서만 생성)·저널 정리의 3경로 수렴·델리게이트 바인딩 규약이 모두 올바르고 CLAUDE.md 규칙 위반은 없다(헤더 내 `GetInstanceDataType()` 정의는 README 가 명시한 규칙 6 예외). 남은 것은 수명주기 대칭·재진입 가드·무음 실패 경로 같은 "아직 밟히지 않은 길"이다. 커버리지: 소스 15파일 전부 통독, 러너 동작 주장은 엔진(UE 5.8) `StateTreeComponent.cpp`·`StateTreeExecutionContext.cpp`·`GameFrameworkComponentManager.cpp` 로 교차 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 러너 콜스택 안에서 `StartQuest` 가 불리면 현재 퀘스트만 정지되고 요청 퀘스트는 무음 유실된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:35-37`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`
- **범주**: 버그/정확성
- **문제**: `ActivateQuest` 의 "ST 실행 콜스택 밖에서만 호출" 전제는 헤더 주석(`WxQuestComponent.h:54`)뿐이고, BlueprintCallable 진입점 `UWxQuestLibrary::StartQuest` 에는 가드가 없다. 퀘스트 트리 안의 BP 태스크가 `StartQuest` 를 직접 부르거나, 퀘스트 태스크의 텔레포트·스폰이 동기 오버랩을 일으켜 트리거 BP 가 `StartQuest` 를 부르면 러너 콜스택 안이 된다. 이때 엔진 동작은 다음과 같다(검증): `StopLogic` 은 재진입이라 정지를 프레임 끝으로 미루고 `bIsRunning=false` 만 세운다(`StateTreeComponent.cpp:246-250`, `StateTreeExecutionContext.cpp:1707-1714`) → `SetStateTreeReference` 는 트리가 아직 Running 이라 교체를 거부한다(`StateTreeComponent.cpp:499`) → `StartLogic` 은 재진입 에러로 무시된다(`:183`). 결과는 "현재 퀘스트는 프레임 끝에 정지돼 저널이 비고, 요청한 퀘스트는 시작되지 않음"이며 모듈 로그 없이 엔진 Warning/Error 만 남는다.
- **제안**: 둘 중 하나. (a) `StopLogic` 직후 `QuestStateTree->GetStateTreeRunStatus() == EStateTreeRunStatus::Running` 이면 정지가 미뤄진 재진입 상황이므로 `RequestActivateQuest(QuestAsset)` 로 폴백하고 리턴. (b) 즉시 경로를 없애고 `StartQuest` 도 다음 틱 예약 경로로 통일(수주 1프레임 지연은 체감 불가, 경로가 하나로 줄어 단순).
- **확신도**: 중간 (엔진 동작은 검증됨, 실제 발생은 호출부 데이터에 달림)

### 2. 🟡 러너 생성·바인딩의 해제 대칭이 없어 컴포넌트 단독 파괴 시 고아 러너가 남는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:117-120`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:79`
- **범주**: 설계/구조 (객체 수명주기)
- **문제**: `BeginPlay` 가 GameState 에 형제 컴포넌트 `QuestStateTree` 를 `NewObject`+`RegisterComponent` 로 만들고 `OnStateTreeRunStatusChanged` 에 바인딩하지만, `EndPlay`/`OnUnregister` 대칭이 없다. 본 컴포넌트는 `UGameFrameworkComponentManager::AddComponentRequest` 로 주입되므로(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:151`) 요청 해제 시 GameState 는 살아 있는 채로 본 컴포넌트만 `DestroyComponent` 된다(엔진 `GameFrameworkComponentManager.cpp:308-351`, `:574`). 그러면 러너는 GameState 에 남아 계속 틱하고, 태스크들은 `FindComponentByClass` 실패로 경고만 반복하며, 재주입 시 같은 고정 이름 `TEXT("QuestStateTree")` 의 `NewObject` 가 살아 있는 등록 컴포넌트를 제자리 교체한다. 현재 GameFeature 비활성은 `UWxExperienceManagerComponent::EndPlay`(월드 정리)에서만 일어나므로(`Source/WxGame/Framework/WxExperienceManagerComponent.cpp:29-37`) 지금은 밟히지 않지만, 런타임 Experience 교체가 들어오는 순간 재현된다.
- **제안**: `EndPlay` 오버라이드에서 `QuestStateTree` 가 있으면 `OnStateTreeRunStatusChanged.RemoveDynamic` → `StopLogic` → `DestroyComponent` → null. 고정 이름 대신 `NAME_None`(자동 이름)으로 바꾸면 재주입 충돌도 없어진다.
- **확신도**: 중간 (현 호출 경로에선 발생하지 않는 잠재 결함)

### 3. 🟡 체인 전이마다 다음 퀘스트 에셋을 게임 스레드에서 동기 로드한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:133`
- **범주**: 성능/안전
- **문제**: `HandleDeferredActivateQuest` 가 `LoadSynchronous()` 로 `UWxQuestStateTree` 를 로드한다. StateTree 에셋은 노드 인스턴스 데이터의 하드 참조(보상 아이템·스폰 클래스 등 크로스모듈 태스크 속성)를 통째로 끌고 오므로, 스트리밍 오픈월드에서 체인 전이 순간 히치가 날 수 있다. 소프트 참조를 택한 이유(타이머 대기 중 GC)는 타당하지만 로드까지 동기일 이유는 없다.
- **제안**: `UAssetManager::GetStreamableManager().RequestAsyncLoad(QuestAsset.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UWxQuestComponent::HandleQuestAssetLoaded, QuestAsset))` 로 바꾸고 콜백에서 `ActivateQuest`. StreamableManager 는 이미 로드된 경우에도 완료 델리게이트를 틱에서 지연 호출하므로(엔진 `StreamableManager.cpp:129-135`, `:1397-1400`) 러너 콜스택 밖 보장이 그대로 유지돼 `SetTimerForNextTick` 은 필요 없어진다. 퀘스트 에셋을 Experience 번들로 선로드하는 정책이 있다면 그대로 둬도 된다.
- **확신도**: 중간 (에셋 의존 그래프 크기에 달림)

### 4. 🟢 컴포넌트 부재·에셋 로드 실패가 로그 없이 삼켜진다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:24-27`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:133`
- **범주**: 버그/정확성 (미처리 실패 경로)
- **문제**: (a) `StartNextQuest::EnterState` 는 컴포넌트 부재 시 로그 없이 `Failed` 만 돌려준다. 헤더(`WxStateTreeTask_StartNextQuest.h:27`)는 "경고를 남기고"라 하지만 cpp 에 로그가 없고, 완료 판정 제외 태스크의 `Failed` 는 엔진이 상태 결과에 반영하지 않으므로(`StateTreeExecutionContext.cpp:3873-3880`) 아무 신호도 나지 않는다. 같은 이유로 `SetQuestTitle`/`SetQuestObjective` 의 `Failed` 반환도 무력하며 경고 로그만이 유일한 신호다. (b) `HandleDeferredActivateQuest` 는 `LoadSynchronous()` 가 null(에셋 삭제·경로 변경)이면 `ActivateQuest(nullptr)` 조기 리턴으로 체인이 무음 단절된다.
- **제안**: (a) `SetQuestTitle` 과 같은 문구의 `UE_LOG(LogWxQuest, Warning, ...)` 추가. (b) 로드 실패 시 소프트 경로를 담은 경고 추가. README 의 "없으면 `Failed` 를 낸다" 규약은 "경고 로그를 남긴다" 로 정정.
- **확신도**: 높음

### 5. 🟢 사용하지 않는 모듈 의존 `GameplayTags`
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`
- **범주**: 중복/복잡도
- **문제**: `GameplayTags` 를 Public 의존으로 두지만 플러그인 소스 어디에도 해당 헤더 include·타입 사용이 없다. `WxCore`(`:20`)도 현재 include 가 없으나 규칙상 허용된 기반 의존이라 유지해도 무방하다.
- **제안**: `GameplayTags` 제거. `WxCore` 는 실제 사용 전까지 빼도 되고 두어도 된다(선택).
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp` (+ 엔진 `StateTreeComponent.cpp`·`StateTreeExecutionContext.cpp`·`StateTreeTasksStatus.h`·`GameFrameworkComponentManager.cpp` 교차 검증)
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_*.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, 소비자 맥락용 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`·`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`
- **미검토 / 한계**: 퀘스트 StateTree 에셋 내부 조립(완료 판정 마스크·태스크 순서, 예: 같은 상태에서 `SetQuestObjective` 가 `SetQuestTitle` 보다 앞이면 목표가 즉시 비워짐)은 데이터 범위라 보지 않았다. 저널 비복제(권위 호스트 전용)와 `WaitMoveToTarget` 의 0번 컨트롤러 전제(데디케이티드 서버에선 임의 클라이언트)는 헤더에 v1 설계로 명시돼 있어 발견에서 제외했다.

---
*문서 기준 커밋 `bd689a19` · 리뷰일 2026-08-22 · 소스 15파일 — `/module-review`로 갱신*
