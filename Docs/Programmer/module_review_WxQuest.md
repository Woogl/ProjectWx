# WxQuest — 코드 리뷰

> 14파일짜리 작은 모듈이고, 권위 모델·에셋 불가지·"상태 수명 = 목표 수명" 규약이 클래스 doc-comment 에 잘 정리돼 있어 전반적으로 건강하다. 남은 위험은 전부 "조용히 어긋나는" 쪽 — StateTree 의 재진입·완료 판정 규약, 그리고 실패 경로가 로그도 종료도 없이 흘러가는 지점들이다. 이번 리뷰는 모듈 소스 14파일(헤더 7 + cpp 7)을 전부 읽고 핵심 로직(`WxQuestComponent.cpp`, 태스크 4종 cpp)을 깊게 봤으며, 구독자 영향 확인용으로 `Source/WxGame/MVVM/WxViewModel_Quest.cpp` 를 참고했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 저널 태스크가 sustained 재진입에 무방비 — 부모 상태에 걸면 전이마다 목표가 뽑혔다 다시 꽂힌다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:10`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:10`
- **범주**: 버그/정확성
- **문제**: 두 태스크의 생성자는 `bShouldCallTick` 과 완료 판정 플래그만 건드리고 `bShouldStateChangeOnReselect` 를 기본값(`true`)으로 둔다. 이 플래그가 참이면 상태가 계속 활성인 채(자식만 바뀌는 sustained 전이) 부모 상태의 태스크에도 `ExitState`/`EnterState` 가 다시 불린다. 그래서 "목표는 부모 상태, 스텝은 자식 상태"라는 자연스러운 조립에서 자식 전이 한 번마다 `RemoveObjective` → `AddObjective` 가 돌아 핸들이 새로 발급되고 `OnJournalChanged` 가 전이당 여러 번 튄다. 구독자는 브로드캐스트마다 목표 뷰모델 UObject 를 통째로 재할당한다(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:51-62`). `SetQuestTitle` 은 더해서 재진입 시 `Objectives.Reset()`(`WxQuestComponent.cpp:53`)까지 다시 돌려 같은 프레임에 저널을 비웠다 채운다. 이는 `WxStateTreeTask_SetQuestObjective.h:28` 이 선언한 "상태에 머무는 동안 유지" 계약과 어긋난다. 같은 모듈의 `WxStateTreeTask_WaitMoveToTarget.cpp:16` 은 이 플래그를 명시적으로 꺼놨으므로, 저널 태스크만 빠진 것으로 보인다.
- **제안**: 두 태스크 생성자에 `bShouldStateChangeOnReselect = false;` 를 추가한다. 재진입 자체는 살리고 싶다면 `EnterState` 초입에서 `Transition.ChangeType == EStateTreeStateChangeType::Sustained` 를 걸러낸다.
- **확신도**: 높음

### 2. 🟡 완료 판정에서 뺀 태스크의 `Failed` 반환이 헤더 문서대로 동작하지 않을 수 있고, `StartNextQuest` 는 로그조차 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:23-26`, `.../WxStateTreeTask_SetQuestTitle.cpp:24-29`, `.../WxStateTreeTask_SetQuestObjective.cpp:24-29`
- **범주**: 버그/정확성
- **문제**: 세 태스크는 생성자에서 `bConsideredForCompletion = false` 로 완료 판정에서 스스로 빠져 놓고, 퀘스트 컴포넌트를 못 찾으면 `EStateTreeRunStatus::Failed` 를 돌려준다. 엔진은 `EnterState` 가 돌려준 완료 상태를 완료 판정 대상 태스크에 대해서만 상태 결과로 채택하므로, 이 조합에서는 실패가 상태에 전파되지 않고 트리가 그대로 굴러갈 가능성이 크다. 즉 헤더 문서(`WxStateTreeTask_StartNextQuest.h:27` "예약 없이 Failed 로 끝난다", `WxStateTreeTask_SetQuestTitle.h:28`, `WxStateTreeTask_SetQuestObjective.h:32`)가 약속한 실패 종료가 실제로는 일어나지 않는다. 제목·목표 태스크는 그래도 `LogWxQuest` 경고를 남기지만 `StartNextQuest` 는 조기 반환에 로그가 하나도 없어(`WxStateTreeTask_StartNextQuest.cpp:23-26`) 퀘스트 체인이 아무 흔적 없이 끊긴다 — 이 부분은 엔진 동작과 무관하게 확정적인 결함이다.
- **제안**: 최소한 `StartNextQuest` 에도 형제 태스크와 같은 `LogWxQuest` 경고를 넣는다. 그 위에, 실패를 상태에 실제로 전파할 생각이면 이 태스크들의 `bConsideredForCompletion = false` 전제를 다시 보고, 유지할 거라면 헤더 문서를 실제 동작대로 정정한다.
- **확신도**: 중간(로그 부재는 높음. 완료 판정 게이트 부분은 이 환경에 엔진 소스가 없어 재확인하지 못했다)

### 3. 🟡 `ActivateQuest` 가 에셋 교체 성공을 확인하지 않고, 그 경로를 BP 에 무방비로 노출한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:32-36`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h:23-24`
- **범주**: 설계/구조
- **문제**: `StopLogic` → `SetStateTreeReference` → `StartLogic` 세 호출 중 어느 것도 결과를 보지 않는다. 러너 실행 콜스택 안에서 불리면 정지가 즉시 반영되지 않아 실행 상태가 `Running` 으로 남고, `UStateTreeComponent::SetStateTreeReference` 는 실행 중 인스턴스에 대한 교체를 경고만 남기고 거부하며, 이어지는 `StartLogic` 도 반려된다. 결과는 크래시가 아니라 "새 퀘스트가 조용히 사라지고 저널만 비는" 상태다. 헤더 주석(`WxQuestComponent.h:54`)은 "ST 실행 콜스택 밖에서만 호출"을 못 박았지만, 실제 저작 진입점인 `UWxQuestLibrary::StartQuest` 는 이 무방비 경로를 BlueprintCallable 로 그대로 노출한다 — 안전한 짝인 `RequestActivateQuest`(`WxQuestComponent.h:58`)는 BP 에서 보이지 않으므로 저작자가 규약을 어길 수단만 있고 지킬 수단이 없다.
- **제안**: `ActivateQuest` 에서 러너가 여전히 실행 중이면(`IsRunning()`) 다음 틱 예약 경로로 흘려보내거나, 최소한 교체 실패를 `LogWxQuest` 로 남긴다.
- **확신도**: 중간(현재 호출자가 전부 트리거 볼륨이면 실제로 재현되지 않을 수 있음)

### 4. 🟡 런타임 생성한 러너의 수명 정리가 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:116-119`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:79`
- **범주**: 설계/구조
- **문제**: `BeginPlay` 에서 `NewObject` + `RegisterComponent` 로 GameState 에 `UStateTreeComponent` 를 붙이는데, `EndPlay` 오버라이드가 없어 `StopLogic`/`DestroyComponent` 를 부르는 곳이 어디에도 없다. 부착이 코드가 아니라 Experience 주입이라(`WxQuestComponent.h:43-44`) Experience 전환·GameFeature 비활성으로 `UWxQuestComponent` 만 제거되는 경로가 존재하는데, 그때 러너는 GameState 에 남아 계속 돌면서 퀘스트의 월드 부수효과(스폰·보상)를 이어가고 태스크는 진입마다 "퀘스트 컴포넌트를 찾지 못함" 경고를 쏟는다. 동적 델리게이트라 파괴된 객체 호출 자체는 걸러지므로 크래시는 아니지만 유령 러너가 남는다.
- **제안**: `EndPlay` 를 오버라이드해 `QuestStateTree` 가 유효하면 `StopLogic` 후 `DestroyComponent` 하고 참조를 비운다.
- **확신도**: 중간(Experience 전환이 실제로 이 컴포넌트를 떼는 경로를 타는지는 미확인)

### 5. 🟡 `bHasActiveQuest` 가 "퀘스트 활성"과 "저널에 제목이 있음"을 겸업한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:101`, `.../Private/Quest/WxQuestComponent.cpp:54`, `:137`
- **범주**: 설계/구조
- **문제**: 이 플래그는 오직 `SetQuestTitle` 에서만 참이 된다(`WxQuestComponent.cpp:54`). 그래서 제목 태스크를 쓰지 않거나 제목을 뒤쪽 상태에서 거는 퀘스트는, 러너가 멀쩡히 돌고 목표까지 저널에 올라와 있는데도 `HasActiveQuest()` 가 false 다. 이 값을 HUD 표시 조건으로 쓰는 구독자(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:46`)에선 "목표는 있는데 저널이 안 뜨는" 형태로 드러난다. 게다가 `ClearJournal` 이 이 플래그로 조기 반환하므로(`WxQuestComponent.cpp:137-140`) 제목 없이 목표만 있던 저널은 종료 시 정리·통지 없이 지나간다. 진짜 "활성" 상태는 러너(`UStateTreeComponent::IsRunning()`)가 갖고 있고, 저널 유무는 `QuestTitle`/`Objectives` 에서 그대로 파생된다.
- **제안**: 이름과 의미를 일치시키거나(저널이 채워졌음을 드러내는 이름), 러너 상태에서 파생시켜 별도 플래그를 없앤다. 최소한 `ClearJournal` 의 조기 반환 조건은 "제목 또는 목표가 하나라도 있으면"으로 넓힌다.
- **확신도**: 중간(모든 퀘스트가 제목 태스크로 시작한다는 저작 관례가 전제라면 의도된 설계일 수 있음)

### 6. 🟡 `WaitMoveToTarget` 은 대상 해석에 실패해도 영원히 Running — 퀘스트가 조용히 멈춘다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23-28`, `:48-54`
- **범주**: 버그/정확성
- **문제**: 빈 로케이터는 진입 시 경고 한 줄을 남기고도 `Running` 을 반환해(`:25-28`) 완료될 수 없는 상태에 그대로 머무른다. 대상이 해석되지 않는 경우(`SyncFind` 가 null, 예: 액터 삭제·WP 셀 미로드·PIE 경로 불일치)도 `:48-52` 에서 아무 로그 없이 Running 을 이어간다. 상태 완료를 내는 유일한 태스크가 이것이므로 결과는 "퀘스트가 그 스텝에서 영구 정지"이고, 로그가 없어 원인 추적도 어렵다. 형제 태스크들이 잘못된 조립에 `Failed` 로 대응하는 것과도 방향이 어긋난다. 이 태스크는 `bConsideredForCompletion` 을 끄지 않았으므로 발견 2 와 달리 `Failed` 반환이 실제로 상태에 전파된다.
- **제안**: 빈 로케이터는 `EnterState` 에서 `Failed` 로 끝낸다. 해석 실패는 최소한 1회성 경고를 남기도록 인스턴스 데이터에 플래그를 두거나, 허용 시간을 넘기면 실패 처리한다.
- **확신도**: 중간(빈 로케이터를 경고만 하고 넘기는 것은 `WxStateTreeTask_WaitMoveToTarget.h:31` 에 명시된 현재 의도이므로, 방침 변경에 해당한다)

### 7. 🟢 퀘스트 컴포넌트 조회가 네 곳에 그대로 복제돼 있고 실패 처리가 제각각이다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:22-23`, `.../WxStateTreeTask_SetQuestObjective.cpp:22-23`·`:41-42`, `.../WxStateTreeTask_StartNextQuest.cpp:21-22`
- **범주**: 중복/복잡도
- **문제**: `Cast<AActor>(Context.GetOwner())` → `FindComponentByClass<UWxQuestComponent>()` 2줄이 네 군데에 동일하게 복사돼 있고, 실패 시 처리만 서로 다르다(경고+Failed / 무시 / 무로그 Failed). 발견 2 의 로그 누락도 이 복제에서 갈라진 것이다. 태스크가 늘어날수록 같은 방식으로 어긋난다.
- **제안**: 모듈 private 헬퍼(예: `WxQuestTaskUtils::FindQuestComponent(Context, TaskName)`)로 조회 + 경고를 한 곳에 모으고 태스크는 반환값만 쓴다.
- **확신도**: 높음

### 8. 🟢 미사용 빌드 의존성 `GameplayTags`
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`
- **범주**: 중복/복잡도
- **문제**: 모듈 소스 전체에 게임플레이 태그 사용이 0건인데 Public 의존성으로 선언돼 있다.
- **제안**: 제거한다.
- **확신도**: 높음

### 9. 🟢 다음 틱 예약 경로의 무검사 `GetWorld()` 와 게임 스레드 동기 로드
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:47`, `:132`
- **범주**: 성능/안전
- **문제**: `GetWorld()->GetTimerManager()` 가 반환값을 검사하지 않고 역참조한다(플레이 중엔 유효하지만 계약상 무방비). 이어지는 `HandleDeferredActivateQuest` 는 `LoadSynchronous()` 로 퀘스트 에셋과 그 하드 참조를 게임 스레드에서 동기 로드하므로 체인 전환 프레임에 히치가 생길 여지가 있다.
- **제안**: `GetWorld()` 널 검사를 추가하고, 히치가 관측되면 `FStreamableManager` 비동기 요청으로 바꾼다.
- **확신도**: 중간(에셋이 작으면 체감되지 않을 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`
- **훑은 파일**: 태스크 4종 헤더, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`, 참고용으로 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`(구독자 영향 확인용, 리뷰 대상 아님)
- **규칙 점검 결과**: `CLAUDE.md` 코딩·모듈 규칙 위반은 발견되지 않았다. 저작권 첫 줄 14/14 준수, `Wx` prefix 준수, 델리게이트·타이머 콜백 `Handle` prefix 준수(`HandleStateTreeRunStatusChanged`, `HandleDeferredActivateQuest`), `BlueprintCallable` 은 `UWxQuestLibrary::StartQuest` 1건뿐이며 BP Function Library 소속, 헤더 인라인 정의는 `GetInstanceDataType()` 4건이고 전부 예외 사유 주석이 붙어 있으며, 람다 사용 0건, 의존 플러그인은 `WxCore` 와 엔진 플러그인뿐이다.
- **미검토 / 한계**: 이 환경에 UE 엔진 소스가 없어 StateTree 내부 동작(발견 1·2·3의 엔진 측 전제)은 API 계약 지식에 근거했을 뿐 소스로 재확인하지 못했다 — 발견 2 를 반영하기 전에 `StateTreeExecutionContext::EnterState` 의 `bConsideredForCompletion` 게이트를 직접 확인할 것. 퀘스트 `UStateTree` 에셋의 실제 상태 구성(부모 상태에 목표를 거는 조립이 쓰이는지)은 BP/에셋 영역이라 보지 않았으므로 발견 1 의 체감 범위는 미정이다. Experience/GameFeature 가 주입 컴포넌트를 떼는 실제 경로(발견 4)도 WxGame 쪽까지 따라가지 않았다. 저널의 리플리케이션 부재는 클래스 문서에 명시된 v1 유보 사항이라 발견으로 잡지 않았다.

---
*문서 기준 커밋 `6ea7624` · 리뷰일 2026-09-06 · 소스 14파일 — `/module-review`로 갱신*
