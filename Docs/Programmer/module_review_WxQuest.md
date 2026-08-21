# WxQuest — 코드 리뷰

> 15파일 규모의 작고 응집도 높은 모듈이다. 권위 경계·저널 수명·러너 재진입 회피 같은 까다로운 지점이 헤더 주석과 README 에 이미 정확히 문서화돼 있어 전반적으로 건강하며, 남은 지적은 대부분 "엔진이 조용히 거절하거나 무시하는 실패 경로를 코드가 성공으로 취급한다" 계열에 몰려 있다. 커버리지: 모듈의 15개 소스를 전부 읽었고, 판정 근거로 엔진 `UStateTreeComponent`·`FStateTreeExecutionContext::Stop`·`FStateTreeReference`·`UGameFrameworkComponentManager` 구현과 `Source/WxGame` 의 소비처(저널 뷰모델, 컴포넌트 주입 액션)까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 `ActivateQuest` 의 세 엔진 호출이 전부 거절 가능한데 반환을 아무도 검사하지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:35-37` (진입점 `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`)
- **범주**: 설계/구조
- **문제**: `StopLogic` → `SetStateTreeReference` → `StartLogic` 은 셋 다 조건부로 아무 일도 하지 않고 반환하는 함수인데, 코드는 세 호출이 항상 관철된다고 가정한다. 러너 실행 콜스택 안에서 호출되면 실제로 이런 연쇄가 난다 — 엔진 `FStateTreeExecutionContext::Stop` 은 재진입 호출 시 정지를 프레임 끝으로 미루고 **`Running` 을 그대로 반환**하며(`Exec.RequestedStop` 에 적재), 그 결과 `UStateTreeComponent::StopTree` 는 상태 변화가 없다고 보아 `OnStateTreeRunStatusChanged` 를 쏘지 않는다(저널 정리 누락) → 이어지는 `SetStateTreeReference` 는 "still Running" 이므로 경고만 남기고 교체를 거부 → `StartLogic` → `StartTree` 는 `CurrentlyRunningExecContext` 가 살아 있어 "Reentrant call is not allowed" 에러로 반환. 순 결과는 **요청한 퀘스트가 조용히 유실되고 러너는 에셋이 그대로인 채 정지 상태로 남는 것**이며, 흔적은 `LogStateTree` 경고 두 줄뿐이다. 반대로 `bIsRunning` 이 false 인데 트리 실행 상태는 Running 인 순간(엔진 `TickComponent` 의 에셋 무효 분기, 재진입 정지 직후)에 걸리면 `StopLogic` 이 조기 반환 → 교체 거부 → `StartLogic` 이 **직전 퀘스트를 처음부터 재시작**시킨다. `ActivateQuest` 는 public 이고 그 유일한 외부 진입점 `UWxQuestLibrary::StartQuest` 는 `BlueprintCallable` 이라, "ST 콜스택 밖에서만 호출" 규약을 지킬 책임이 전적으로 BP 작성자에게 있다. 특히 `OnJournalChanged` 는 `BlueprintAssignable` 이면서 `EnterState` 안에서 broadcast 되므로(`WxQuestComponent.cpp:56` ← `WxStateTreeTask_SetQuestTitle.cpp:32`), 이 델리게이트를 받아 `StartQuest` 를 부르는 BP 하나면 규약이 깨진다.
- **제안**: `ActivateQuest` 를 private 으로 내리고 모든 외부 진입점을 `RequestActivateQuest`(다음 틱) 로 일원화해 규약 자체를 없앤다 — 체인 경로가 이미 그 1틱 지연을 감수하고 있다. 추가로 `SetStateTreeReference` 직후 러너의 실제 참조가 요청한 에셋인지 확인하고, 어긋나면 `StartLogic` 을 건너뛴 뒤 `LogWxQuest` Error 로 호출 지점을 지목한다.
- **확신도**: 높음

### 2. 🟡 `SetQuestTitle` 이 목표 목록을 통째로 비워, 아직 살아 있는 목표가 복구 불가능하게 사라진다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:51-57`
- **범주**: 버그/정확성
- **문제**: `SetQuestTitle` 이 `Objectives.Reset()` 으로 목록을 비우지만, 그 목표를 발급한 `FWxStateTreeTask_SetQuestObjective` 인스턴스들은 여전히 자기 상태에 살아 있다. 목표는 `EnterState` 에서만 등록되므로 한 번 지워지면 그 상태가 끝날 때까지 되살아날 길이 없고, 나중에 도는 `ExitState` 의 `RemoveObjective` 는 없는 핸들을 조용히 무시하도록 돼 있어(`:70-82`) 아무 진단도 남지 않는다. 실제로 물리는 조립은 두 가지다 — (a) 같은 상태 안에서 목표 태스크를 제목 태스크보다 위에 배치, (b) 퀘스트 중간에 제목을 갱신하려고 하위 상태에서 제목 태스크를 한 번 더 사용(상위 상태가 건 목표가 전멸). 둘 다 에디터에서 막히지 않는다.
- **제안**: `SetQuestTitle` 에서 `Objectives.Reset()` 을 빼고, 저널 비우기를 러너 종료 한 곳(`ClearJournal`)으로 일원화한다. 제목 재설정 시 목표를 비우는 것이 의도라면, 남은 핸들이 있는 상태에서의 Reset 은 `LogWxQuest` 경고를 남겨 조립 실수를 드러낸다.
- **확신도**: 중간

### 3. 🟡 퀘스트 체인 전환 때 게임 스레드에서 에셋을 동기 로드한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:131-134`
- **범주**: 성능/안전
- **문제**: `HandleDeferredActivateQuest` 가 `LoadSynchronous()` 로 다음 퀘스트 `UWxQuestStateTree` 를 게임 스레드에서 끌어온다. 이 참조를 `TSoftObjectPtr` 로 둔 목적 자체가 미리 로드하지 않는 것이므로 콜드 로드가 기본값이고, 로드 대상은 ST 에셋 하나가 아니라 그 안 태스크 인스턴스 데이터가 하드 참조하는 것들(크로스모듈 태스크의 액터 클래스·보상 테이블 등)까지다. 하필 시점이 퀘스트 단계 완료 직후라 다른 연출과 겹치기 쉽고, 오픈월드 인게임 중이라 히치가 그대로 노출된다.
- **제안**: `FStreamableManager::RequestAsyncLoad` 로 바꾸고 핸들을 멤버로 보관해 완료 콜백에서 `ActivateQuest` 를 호출한다(핸들 보관이 곧 GC 방지라 다음 틱 타이머의 원래 목적도 함께 만족한다).
- **확신도**: 중간

### 4. 🟢 `StartNextQuest` 만 조립 오류를 진단 없이 삼키고, 헤더 주석은 경고를 남긴다고 잘못 적혀 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:24-27`
- **범주**: 버그/정확성
- **문제**: 컴포넌트를 못 찾으면 로그 없이 `Failed` 만 반환한다. 형제 태스크는 같은 상황에서 오너 이름까지 찍은 경고를 남기며(`WxStateTreeTask_SetQuestTitle.cpp:26`, `WxStateTreeTask_SetQuestObjective.cpp:26`), 이 태스크의 헤더 주석 `WxStateTreeTask_StartNextQuest.h:27` 도 "경고를 남기고 예약하지 않는다"고 적혀 있어 코드와 어긋난다. 이 태스크의 실패는 퀘스트 체인이 끊긴다는 뜻이라 오히려 가장 진단이 필요한 지점이다.
- **제안**: 형제 태스크와 동일한 형태의 `UE_LOG(LogWxQuest, Warning, ...)` 를 추가한다(`WxQuestModule.h` include 필요).
- **확신도**: 높음

### 5. 🟢 도달 대상이 비면 퀘스트가 경고 한 줄만 남기고 영구 정지한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23-28`
- **범주**: 설계/구조
- **문제**: 빈 로케이터에서 경고를 남기고도 `Running` 을 반환하고, `Tick` 은 `SyncFind` 가 계속 null 이라 영원히 `Running` 을 반환한다(`:48-54`). 이 태스크가 상태 완료를 내는 유일한 축이므로 결과는 되돌릴 수 없는 소프트락이고 다음 체인까지 함께 죽는다. 나머지 세 태스크는 같은 범주의 조립 오류를 전부 `Failed` 로 즉시 드러내므로 실패 정책이 이 태스크에서만 갈라진다.
- **제안**: 빈 로케이터는 `EnterState` 에서 `Failed` 를 반환해 정책을 맞춘다 — 러너가 멈추면 `HandleStateTreeRunStatusChanged` 가 저널을 걷어가 조립 오류가 바로 눈에 띈다. 대상이 스트리밍 아웃돼 일시적으로 해석되지 않는 경우는 지금처럼 대기를 유지하면 된다.
- **확신도**: 중간 (헤더 주석이 "경고를 남긴다"를 명시적 선택으로 적어 두어 의도일 수 있음)

### 6. 🟢 `bHasActiveQuest` 가 "제목이 등록됨"을 "퀘스트가 활성"의 대용으로 쓴다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:55`, `:84-87`, `:136-141`
- **범주**: 설계/구조
- **문제**: 이 플래그를 켜는 곳이 `SetQuestTitle` 하나뿐이다. 제목 태스크를 두지 않은 퀘스트(예: 목표만 갱신하는 짧은 단계)는 목표가 실제로 걸려 있어도 `HasActiveQuest()` 가 false 라 HUD 뷰모델이 저널을 숨기고(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:50`), 게다가 `ClearJournal` 이 이 플래그를 게이트로 조기 반환하므로(`:138-141`) 종료 시 정리 통지조차 나가지 않는다. 러너의 실행 여부라는 진짜 근거를 이미 컴포넌트가 쥐고 있는데 별도 플래그로 근사한 형태다.
- **제안**: 플래그를 없애고 `HasActiveQuest()` 를 `QuestStateTree && QuestStateTree->GetStateTreeRunStatus() == EStateTreeRunStatus::Running` 로 파생시킨다. `ClearJournal` 의 조기 반환은 제목·목표가 이미 비었는지로 판정한다.
- **확신도**: 중간

### 7. 🟢 런타임 생성한 러너 컴포넌트에 대칭 정리가 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:106-121`
- **범주**: 설계/구조
- **문제**: `BeginPlay` 가 오너에 `UStateTreeComponent` 를 만들어 `RegisterComponent` 하고 델리게이트까지 걸지만, `EndPlay`·`OnUnregister` 오버라이드가 없어(헤더의 유일한 오버라이드가 `BeginPlay` — `Public/Quest/WxQuestComponent.h:79`) 소유권이 한 방향으로만 성립한다. 본 컴포넌트는 코드가 아니라 주입으로 붙고, 프로젝트 자신의 주입 액션이 GameFeature 비활성 시 요청 핸들을 놓아 주입 컴포넌트를 파괴한다(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:78` → 엔진 `UGameFrameworkComponentManager::RemoveComponentRequest` → `DestroyInstancedComponent`). 반면 러너는 매니저가 추적하지 않으므로 GameState 에 남아 계속 틱하며 퀘스트 태스크(스폰·보상 등 월드 부수효과)를 구동한다. 재주입 시엔 고정 이름 `TEXT("QuestStateTree")` 로 같은 Outer 에 `NewObject` 하므로 살아 있는 기존 러너와 이름이 충돌한다. 현재는 비활성이 `UWxExperienceManagerComponent::EndPlay`(월드 티어다운) 에서만 일어나 실피해가 없는 잠복 결함이지만, 인게임 Experience 교체가 생기는 순간 드러난다.
- **제안**: `EndPlay`(또는 `OnUnregister`) 에서 `OnStateTreeRunStatusChanged` 바인딩을 풀고 `StopLogic` 후 러너를 `DestroyComponent` 한다.
- **확신도**: 중간

### 8. 🟢 사용하지 않는 빌드 의존 `GameplayTags`
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`
- **범주**: 중복/복잡도
- **문제**: 모듈 전체에 `GameplayTag` 사용이 한 건도 없다. (같은 목록의 `WxCore` 역시 include 가 없지만 전 플러그인이 일괄로 싣는 프로젝트 관례이므로 별건으로 보지 않는다.)
- **제안**: 항목을 지운다.
- **확신도**: 높음

### 9. 🟢 저널이 복제되지 않아 데디케이티드 구성의 클라이언트에선 퀘스트 UI 가 항상 빈다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:95-102`
- **범주**: 설계/구조
- **문제**: `QuestTitle`·`Objectives`·`bHasActiveQuest` 가 전부 비복제 평범 멤버이고 저널을 채우는 코드는 권위 러너 안에만 있다. 컴포넌트 사본과 뷰모델 리졸버는 클라이언트에도 그대로 붙으므로(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:72`) 구독은 성립하는데 통지가 영원히 오지 않는, 진단이 어려운 무증상 실패가 된다. 헤더 주석과 README 가 v1 싱글/리슨 호스트 전제를 명시하고 있어 알려진 보류 사항이지만, 멀티 정책을 켤 때 손봐야 할 지점을 여기 남긴다.
- **제안**: 멀티 대응 시점에 세 필드를 `Replicated` 로 올리고(`FWxQuestObjective` 는 이미 USTRUCT) `OnRep` 에서 `OnJournalChanged` 를 broadcast 한다. 그 전까지는 비-권위에서 뷰모델 생성 자체를 막아 "구독은 됐는데 영영 빈 화면"을 피하는 편이 낫다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 문서화된 v1 범위 결정)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h`, 태스크 헤더 4종, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`
- **참고로 읽은 모듈 밖 코드**: `Source/WxGame/MVVM/WxViewModel_Quest.cpp`(저널 소비처), `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`·`Source/WxGame/Framework/WxExperienceManagerComponent.cpp`(부착·회수 경로), 엔진 `StateTreeComponent.cpp`·`StateTreeExecutionContext.cpp`·`StateTreeReference.h`(발견 1·7 의 판정 근거)
- **미검토 / 한계**:
  - 규칙 점검 결과 위반 없음 — 저작권 첫 줄(15/15)·`Wx` prefix·`Handle` 콜백 prefix·`Super::BeginPlay` 호출·`BlueprintCallable` 사용처(BP Function Library 한 곳뿐)·람다 부재(0건)·`WxCore` 외 Wx 플러그인 참조 없음 모두 충족. 태스크 헤더의 `GetInstanceDataType()` 인라인 정의 4건은 각 헤더가 코딩 규칙 6 의 예외임을 명시해 두어 발견으로 올리지 않았다(다만 `CLAUDE.md` 규칙 6 에는 그 예외가 적혀 있지 않아, 매 리뷰마다 재판정되는 것을 막으려면 규칙 쪽에 한 줄 명문화하는 편이 낫다).
  - 퀘스트 StateTree 에셋(`/Game/Quest/**`)의 상태 구성·태스크 배치는 BP/에셋 영역이라 범위 밖이다. 특히 "스스로 끝나야 하는 상태에는 대기 태스크를 하나 둔다"는 README 규약과 발견 2 의 태스크 배치 순서 문제는 코드로 준수 여부를 확인할 수 없다.
  - `FUniversalObjectLocator::SyncFind` 와 `UGameplayStatics::GetPlayerController` 를 캐시 없이 매 틱 도는 비용(`WxStateTreeTask_WaitMoveToTarget.cpp:41-48`)은 동시 활성 대기 태스크가 1개뿐인 구조상 경로 조회 수준으로 판단해 성능 지적에서 제외했다(실측하지 않음).
  - `RequestActivateQuest` 의 무검증 `GetWorld()` 역참조(`WxQuestComponent.cpp:48`)는 러너가 살아 있는 동안 오너 월드가 항상 유효하므로 발견으로 올리지 않았다.

---
*문서 기준 커밋 `ce04ce1f` · 리뷰일 2026-08-21 · 소스 15파일 — `/module-review`로 갱신*
