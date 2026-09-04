# WxQuest — 코드 리뷰

> 14파일짜리 작은 모듈이고 권위 모델·에셋 불가지·저널 수명 규약이 클래스 doc-comment에 잘 정리돼 있어 전반적으로 건강하다. 다만 StateTree 태스크가 엔진의 재진입(sustained) 규약과 완료 판정 규약을 덜 반영해 조용히 어긋나는 지점이 몇 군데 있다. 이번 리뷰는 모듈 전체 소스 14파일을 모두 읽고, 판단이 갈리는 부분은 UE 5.8 엔진 소스(`StateTreeExecutionContext.cpp`, `StateTreeComponent.cpp`)로 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 저널 태스크가 sustained 재진입에 무방비 — 부모 상태에 걸면 전이마다 목표가 뽑혔다 다시 꽂힌다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:10`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:10`
- **범주**: 버그/정확성
- **문제**: 두 태스크 모두 생성자에서 `bShouldStateChangeOnReselect`를 건드리지 않아 기본값 `true`다. 엔진은 이 플래그가 true인 태스크에 대해, 상태가 계속 활성이더라도(`EStateTreeStateChangeType::Sustained`) `ExitState`/`EnterState`를 다시 호출한다(`StateTreeExecutionContext.cpp:4029`, `:3839`). 따라서 목표를 부모 상태에 걸고 자식 상태들이 스텝을 밟는 자연스러운 조립에서, 자식 전이 한 번마다 `RemoveObjective` → `AddObjective`가 돌아 핸들이 매번 새로 발급되고 `OnJournalChanged`가 전이당 여러 번 튄다. 구독자(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:56`)는 브로드캐스트마다 목표 뷰모델 UObject를 통째로 재할당하므로 비용도 그대로 따라온다. `SetQuestTitle`은 더해서 재진입 시 `Objectives.Reset()`(`WxQuestComponent.cpp:53`)을 다시 돌려 같은 프레임 안에서 저널을 비웠다 채운다. 이는 `WxStateTreeTask_SetQuestObjective.h:28`이 선언한 "상태에 머무는 동안 유지" 계약과 어긋나고, 엔진 헤더 주석이 명시한 용법("자식 상태에서 확보가 유지되는 자원형 태스크는 false", `StateTreeTaskBase.h:108`)과도 반대다. 같은 모듈의 `WxStateTreeTask_WaitMoveToTarget.cpp:16`은 이 플래그를 명시적으로 꺼놨어서, 저널 태스크만 빠진 것으로 보인다.
- **제안**: 두 태스크 생성자에 `bShouldStateChangeOnReselect = false;`를 추가한다(의도적으로 재진입시키고 싶다면 `Transition.ChangeType == EStateTreeStateChangeType::Changed` 가드).
- **확신도**: 높음

### 2. 🟡 완료 판정에서 뺀 태스크의 `Failed` 반환은 엔진이 버린다 — 문서와 실제 동작 불일치
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:23`, `.../WxStateTreeTask_SetQuestTitle.cpp:28`, `.../WxStateTreeTask_SetQuestObjective.cpp:28`
- **범주**: 버그/정확성
- **문제**: 세 태스크는 생성자에서 `bConsideredForCompletion = false`로 완료 판정에서 빠지는데, 엔진은 `EnterState`가 돌려준 상태를 `IsConsideredForCompletion()` 게이트 안에서만 결과에 반영한다(`StateTreeExecutionContext.cpp:3873`). 즉 퀘스트 컴포넌트를 못 찾아 `EStateTreeRunStatus::Failed`를 돌려줘도 상태는 정상 진입한 것으로 계속 굴러간다. 헤더 문서(`WxStateTreeTask_StartNextQuest.h:27` "예약 없이 Failed 로 끝난다", `WxStateTreeTask_SetQuestTitle.h:28`, `WxStateTreeTask_SetQuestObjective.h:32`)가 약속한 실패 종료가 실제로는 일어나지 않는다. 제목·목표 태스크는 그래도 경고 로그가 남지만, `StartNextQuest`는 로그조차 없어 퀘스트 체인이 아무 흔적 없이 끊긴다.
- **제안**: 실패를 실제로 상태에 전파할 생각이면 이 태스크들의 완료 판정 제외 전제를 다시 볼 것. 그게 아니라면(현 설계 유지) 헤더 문서를 실제 동작대로 고치고, 최소한 `StartNextQuest`에도 형제 태스크와 같은 `LogWxQuest` 경고를 남긴다.
- **확신도**: 높음

### 3. 🟡 `ActivateQuest`가 에셋 교체 성공을 확인하지 않고, 그 경로를 BP에 무방비로 노출한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:32`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h:23`
- **범주**: 설계/구조
- **문제**: `StopLogic` → `SetStateTreeReference` → `StartLogic` 세 호출 중 어느 것도 결과를 보지 않는다. 러너 실행 콜스택 안에서 불리면 `Stop`이 프레임 끝으로 연기되면서 실행 상태가 그대로 `Running`으로 남고(`StateTreeExecutionContext.cpp:1707`), 이어지는 `SetStateTreeReference`는 "Trying to change the state tree on a running instance" 경고만 남기고 거부되며(`StateTreeComponent.cpp:491`), `StartLogic`도 재진입 에러로 반려된다. 결과는 크래시가 아니라 "퀘스트가 조용히 사라짐"이다. 헤더 주석(`WxQuestComponent.h:54`)은 콜스택 밖에서만 부르라고 못 박았지만, 실제 저작 진입점인 `UWxQuestLibrary::StartQuest`는 BlueprintCallable로 이 무방비 경로를 그대로 노출한다 — 안전한 짝인 `RequestActivateQuest`는 BP에서 볼 수 없다.
- **제안**: `ActivateQuest`에서 교체 후 러너가 실제로 요청한 에셋으로 시작했는지 확인해 실패를 `LogWxQuest`로 남기거나, `UStateTreeComponent::IsRunning()`이 참일 때는 다음 틱 예약 경로로 흘려보낸다.
- **확신도**: 중간(현재 호출자가 전부 트리거 볼륨이면 실제로 재현되지 않을 수 있음)

### 4. 🟡 런타임 생성한 러너의 수명 정리가 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:116`
- **범주**: 설계/구조
- **문제**: `BeginPlay`에서 `NewObject`+`RegisterComponent`로 GameState에 `UStateTreeComponent`를 붙이지만 `EndPlay`를 오버라이드하지 않아 `DestroyComponent`/`StopLogic`을 부르는 곳이 없다. 컴포넌트 부착이 코드가 아니라 Experience 주입이라(`WxQuestComponent.h:43`) Experience 전환·GameFeature 비활성으로 `UWxQuestComponent`만 제거되는 경로가 존재하는데, 이때 러너는 GameState에 그대로 남아 계속 돌면서 퀘스트의 월드 부수효과(스폰·보상)를 이어가고, 태스크들은 매 진입마다 "퀘스트 컴포넌트를 찾지 못함" 경고를 쏟는다. 델리게이트는 동적 델리게이트라 파괴된 객체 호출은 걸러지므로 크래시는 아니지만, 유령 러너가 남는 것 자체가 문제다.
- **제안**: `EndPlay`에서 `QuestStateTree`가 유효하면 `StopLogic` 후 `DestroyComponent`하고 참조를 비운다.
- **확신도**: 중간(Experience 전환 중 컴포넌트 제거 경로를 실제로 타는지는 미확인)

### 5. 🟡 `bHasActiveQuest`가 "퀘스트 활성"과 "저널에 제목이 있음"을 겸업한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:101`, `.../Private/Quest/WxQuestComponent.cpp:54`
- **범주**: 설계/구조
- **문제**: 이 플래그는 오직 `SetQuestTitle`에서만 참이 된다. 그래서 제목 태스크를 쓰지 않거나 제목을 뒤쪽 상태에서 거는 퀘스트는, 러너가 멀쩡히 돌고 목표까지 저널에 올라와 있는데도 `HasActiveQuest()`가 false다. 이 값을 HUD 표시 조건으로 쓰는 구독자(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:46`)에선 목표가 있는데 저널이 안 뜨는 형태로 드러난다. 또한 `ClearJournal`이 이 플래그로 조기 반환하므로(`WxQuestComponent.cpp:137`) 제목 없이 목표만 있던 저널은 정리·통지 없이 지나간다. 실제 권위 있는 "활성" 상태는 러너(`UStateTreeComponent::IsRunning()`)가 갖고 있고, 저널 유무는 `QuestTitle`/`Objectives`에서 그대로 파생된다.
- **제안**: 이름과 의미를 일치시키거나(예: 저널 채워짐 여부임을 드러내는 이름) 러너 상태에서 파생시켜 별도 플래그를 없앤다.
- **확신도**: 중간(모든 퀘스트가 제목 태스크로 시작한다는 저작 관례가 전제라면 의도된 설계일 수 있음)

### 6. 🟢 미사용 빌드 의존성 `GameplayTags`
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`
- **범주**: 중복/복잡도
- **문제**: 모듈 소스 전체에 게임플레이 태그 사용이 0건인데 Public 의존성으로 선언돼 있다.
- **제안**: 제거한다.
- **확신도**: 높음

### 7. 🟢 다음 틱 예약 경로의 무검사 `GetWorld()`와 게임 스레드 동기 로드
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:47`, `:132`
- **범주**: 성능/안전
- **문제**: `GetWorld()->GetTimerManager()`는 반환값을 검사하지 않고 역참조한다(플레이 중엔 유효하나 계약상 무방비). 이어지는 `HandleDeferredActivateQuest`는 `LoadSynchronous()`로 퀘스트 에셋과 그 하드 참조를 게임 스레드에서 동기 로드하므로, 체인 전환 프레임에 히치가 생길 여지가 있다.
- **제안**: `GetWorld()` 널 검사를 추가하고, 히치가 관측되면 `FStreamableManager` 비동기 요청으로 바꾼다.
- **확신도**: 중간(에셋이 작으면 체감되지 않을 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, 태스크 4종 헤더, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, 참고용으로 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`(구독자 영향 확인용, 리뷰 대상 아님)
- **규칙 점검 결과**: CLAUDE.md 코딩·모듈 규칙 위반은 발견되지 않았다. 저작권 첫 줄 14/14 준수, `Wx` prefix 준수, 델리게이트·타이머 콜백 `Handle` prefix 준수, `BlueprintCallable`은 BP Function Library 1건뿐, 인라인 정의는 `GetInstanceDataType()` 4건이며 전부 예외 사유 주석이 붙어 있고, 플러그인 참조는 `WxCore` 외 Wx 플러그인이 없다.
- **미검토 / 한계**: 퀘스트 `UStateTree` 에셋의 실제 상태 구성(부모 상태에 목표를 거는 조립이 실제로 쓰이는지)은 확인하지 않아 발견 1의 체감 영향 범위는 미정이다. Experience/GameFeature가 주입 컴포넌트를 제거하는 실제 경로(발견 4)도 WxGame 쪽 코드까지 따라가지 않았다. 멀티플레이 저널 복제 부재는 클래스 문서에 명시된 v1 유보 사항이라 발견으로 잡지 않았다.

---
*문서 기준 커밋 `491dd7ec` · 리뷰일 2026-09-05 · 소스 14파일 — `/module-review`로 갱신*
