# WxQuest — 코드 리뷰

> 14파일뿐인 작고 응집도 높은 모듈이다. 러너 소유·권위 게이팅·저널 정리 수렴이 헤더 주석과 실제 코드가 일치하고, 프로젝트 코딩·모듈 규칙 위반은 하나도 없다. 심각한 결함은 발견되지 않았으며 남은 것은 재진입 규약의 강제력과 로딩·진단 수준의 개선점이다. 이번 리뷰는 `Plugins/WxQuest` 소스 14파일 전부를 읽고, 판단이 엔진 동작에 걸린 지점(`UStateTreeComponent`의 Start/Stop/SetStateTreeReference 재진입 규칙, `bConsideredForCompletion` 의 에디터 전용 여부와 EnterState 실패 취급, ExitState 호출 조건)은 UE 5.8 엔진 소스와 엔진 테스트로 직접 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 BP 에 열린 `StartQuest` 가 러너 실행 콜스택 안에서 불리면 퀘스트가 조용히 죽는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:34`
- **범주**: 설계/구조
- **문제**: `ActivateQuest` 는 `StopLogic → SetStateTreeReference → StartLogic` 3연타인데, 이 셋 모두 러너 실행 중 호출에 대한 방어가 엔진 쪽에 있고 그 방어가 전부 "로그 남기고 리턴"이다. UE 5.8 `UStateTreeComponent` 기준으로 `StopLogic` 은 `CurrentlyRunningExecContext` 가 있으면 실제 정지를 프레임 끝으로 미루고, `SetStateTreeReference` 는 인스턴스가 Running 이면 경고만 남기고 교체를 거부하며, `StartTree` 는 재진입이면 "Reentrant call" 에러 후 리턴한다. 즉 이 경로로 들어오면 **기존 퀘스트만 정지되고 새 퀘스트는 시작되지 않아** 저널이 빈 채로 시스템이 멈춘다. 컴포넌트 헤더는 "ST 실행 콜스택 밖에서만 호출한다"는 규약을 달아 두었지만, 정작 BP 에 노출된 `UWxQuestLibrary::StartQuest` 는 하드 포인터로 `ActivateQuest` 를 직결하고 라이브러리 헤더에는 그 제약이 적혀 있지 않다. 안전한 짝인 `RequestActivateQuest` 는 BP 에 노출돼 있지 않아, BP 작업자가 고를 수 있는 유일한 진입점이 위험한 쪽이다.
- **제안**: `StartQuest` 도 다음 틱 예약 경로로 보내 두 경우 모두 안전하게 만든다(레벨 트리거에서 부를 때 한 프레임 늦어지는 것 외엔 손해가 없다). 그게 과하면 최소한 라이브러리 헤더에 "퀘스트 StateTree 안에서 부르지 말 것"을 명시한다.
- **확신도**: 중간(호출 지점을 레벨 트리거로 한정하는 현재 규약이 지켜지는 동안에는 발생하지 않는다)

### 2. 🟡 체인 전환마다 다음 퀘스트 에셋을 게임 스레드에서 동기 로드한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:132`
- **범주**: 성능/안전
- **문제**: `HandleDeferredActivateQuest` 가 `LoadSynchronous()` 로 다음 퀘스트 StateTree 를 블로킹 로드한다. 소프트 참조로 넘겨 GC 를 피한 설계 의도는 타당하나, 로드 자체가 동기라 퀘스트 트리와 그 태스크들이 하드 참조하는 것들(README 가 언급하는 GiveRewards 계열이 참조할 보상 아이템·스폰 액터 등)이 전부 딸려 오면 체인이 넘어가는 순간 히치가 난다. 퀘스트 전환은 컷신·대화 직후 같은 눈에 띄는 타이밍에 몰린다.
- **제안**: `UAssetManager::GetStreamableManager().RequestAsyncLoad` 로 비동기 로드 후 완료 콜백에서 `ActivateQuest` 를 호출한다. 지금 구조가 이미 "다음 틱"으로 한 번 미루고 있어 비동기로 바꿔도 호출 규약이 달라지지 않는다.
- **확신도**: 중간(현재 퀘스트 트리들이 가벼우면 체감되지 않을 수 있다 — 실측 후 판단할 여지가 있다)

### 3. 🟢 `StartNextQuest` 의 실패 경로만 완전히 무음이라 끊긴 체인이 흔적을 남기지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:25`
- **범주**: 버그/정확성
- **문제**: 형제 태스크인 `SetQuestTitle`·`SetQuestObjective` 는 같은 상황(오너에서 퀘스트 컴포넌트를 못 찾음)에서 `LogWxQuest` 경고를 남기는데, `StartNextQuest` 만 조용히 `Failed` 를 돌려준다. 그런데 이 태스크는 `bConsideredForCompletion = false` 라서 엔진이 그 `Failed` 를 완료 판정에서 아예 무시한다(UE 5.8 엔진 테스트 `StateTreeTaskStateTest.cpp` 의 `FStateTreeTest_TasksCompletion_IneligibleTaskEnterStateFail` 이 "ineligible EnterState failure is ignored" 로 명시). 결과적으로 잘못 조립된 체인은 로그도 상태 실패도 없이 그냥 다음 퀘스트로 안 넘어간다.
- **제안**: 형제 태스크와 같은 형태로 `UE_LOG(LogWxQuest, Warning, ...)` 한 줄을 맞춰 준다.
- **확신도**: 높음

### 4. 🟢 저널이 체인 전환 사이 한 프레임 비었다가 다시 채워진다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:47`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:122`
- **범주**: 설계/구조
- **문제**: 퀘스트 A 의 트리가 스스로 완료되는 프레임에 실행 상태가 Running 을 벗어나 `ClearJournal` 이 돌고(`bHasActiveQuest=false`, `OnJournalChanged` 발화), 예약된 퀘스트 B 활성화는 다음 틱 타이머라 그 프레임 안에서는 저널이 비어 있다. HUD 뷰모델은 그 발화를 그대로 받아 pull 하므로 퀘스트 패널이 1프레임 사라졌다가 돌아온다. 특히 `StartNextQuest` 만 있는 종단 상태는 완료 판정 대상 태스크가 0개라 진입 즉시 완료되므로(기존에 확인된 5.8 동작) 이 경로가 체인의 표준 형태다.
- **제안**: 기능상 문제는 아니라 방치해도 되지만, 패널에 등장·퇴장 연출이 붙는 순간 깜빡임으로 보인다. 고친다면 저널 정리 자체를 미루기보다 HUD 쪽에서 한 프레임 디바운스하는 편이 컴포넌트에 분기용 상태를 새로 들이지 않아 깔끔하다.
- **확신도**: 중간(연출이 없는 지금은 눈에 안 띌 수 있다)

### 5. 🟢 `GameplayTags` 는 쓰이지 않는 빌드 의존성이다
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`
- **범주**: 중복/복잡도
- **문제**: 모듈 소스 전체에 `FGameplayTag`·태그 관련 사용처가 한 곳도 없다(모듈 내 유일한 "GameplayTag" 문자열이 이 의존성 선언 자신이다).
- **제안**: 줄을 지운다. 나중에 퀘스트 조건을 태그로 표현하게 되면 그때 다시 넣으면 된다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin` — 소비자 쪽 계약 확인용으로 `Source/WxGame/MVVM/WxViewModel_Quest.cpp` 도 읽었다(WxGame 은 게임 모듈이라 WxQuest 참조는 규칙 위반이 아니다)
- **확인했고 문제 없었던 것**:
  - 코딩 규칙 전 항목 통과 — 모든 파일 첫 줄 저작권 표기, `Wx` 접두사, 델리게이트 콜백의 `Handle` 접두사(`HandleStateTreeRunStatusChanged`·`HandleDeferredActivateQuest`), `BlueprintCallable` 은 Blueprint Function Library 인 `UWxQuestLibrary` 한 곳뿐, 람다 사용 없음, 헤더 인라인 정의는 4개 태스크의 `GetInstanceDataType()` 뿐이고 네 파일 모두 예외 사유 주석이 달려 있음.
  - 모듈 경계 — 빌드 의존성에 `WxCore` 외 다른 Wx 플러그인이 없다.
  - `bConsideredForCompletion` 을 `#if WITH_EDITORONLY_DATA` 로 감싼 것은 **정상**이다. UE 5.8 `FStateTreeTaskBase` 에서 이 필드 자체가 에디터 전용이고 컴파일러가 상태의 `CompletionTasksMask` 로 구워 넣으므로, 쿠킹 빌드에서 설정 태스크가 완료 판정에 복귀하는 일은 없다.
  - `EnterState` 가 `Failed` 를 반환한 태스크에도 `ExitState` 는 호출된다(엔진 `FStateTreeExecutionContext::ExitState` 는 `EnterState` 가 불린 범위 전체를 역순 순회). 따라서 목표를 걸었다 걷어가는 짝은 실패 경로에서도 새지 않는다. `RemoveObjective(INDEX_NONE)` 도 핸들이 0부터 발급되므로 절대 오매치하지 않는다.
  - `BeginPlay` 의 러너 생성 순서가 맞다 — `SetStartLogicAutomatically(false)` 를 `RegisterComponent()` 앞에 둬서, 이미 BeginPlay 를 마친 오너에 붙는 컴포넌트가 자동 시작되는 것을 막는다.
  - `NewObject<UStateTreeComponent>(Owner, TEXT("QuestStateTree"))` 의 고정 이름은 중복 생성 시 위험하지만, 주입이 `UGameFrameworkComponentManager::AddComponentRequest` 를 거쳐 (Receiver, Component) 쌍으로 중복 제거되므로 같은 GameState 에 `UWxQuestComponent` 가 두 벌 붙는 경로가 실재하지 않는다.
- **미검토 / 한계**: 저널 미복제(데디케이티드 서버·클라 HUD)는 이미 보류로 결정된 사안이라 발견으로 올리지 않았다. 퀘스트 `UStateTree` 에셋 자체의 조립(상태 구성·전이 설정)과 `GiveRewards` 등 WxQuest 밖에 있는 크로스모듈 태스크는 범위 밖이며, 그쪽 하드 참조 규모는 2번 발견의 영향도를 좌우하므로 실측 시 함께 봐야 한다.

---
*문서 기준 커밋 `ba33d69e` · 리뷰일 2026-09-01 · 소스 14파일 — `/module-review`로 갱신*
