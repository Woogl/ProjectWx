# WxQuest — 코드 리뷰

> 14파일뿐인 작고 응집도 높은 모듈이다. 러너 소유·권위 게이팅·저널 정리 수렴이 헤더 주석과 실제 코드가 일치하고, 프로젝트 코딩·모듈 규칙 위반은 하나도 없다. 심각한 결함은 없으며 남은 것은 저널 소유권, 재진입 규약의 강제력, 로딩·진단 수준의 개선점이다. 이번 리뷰는 `Plugins/WxQuest` 소스 14파일 전부를 읽고, 판단이 엔진 동작에 걸린 지점(`UStateTreeComponent` 의 Start/Stop/SetStateTreeReference 재진입 규칙, 상태 재선택 시 태스크 Enter/ExitState 재호출 조건, `bConsideredForCompletion` 의 에디터 전용 여부와 EnterState 실패 취급, `FUniversalObjectLocator` 의 액터 해석 경로)은 UE 5.8 엔진 소스와 엔진 테스트로 직접 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 제목 설정 태스크가 목표 목록까지 비우는데, 그 태스크는 스텝이 넘어갈 때마다 다시 실행된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:53`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:10`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:10`
- **범주**: 버그/정확성
- **문제**: `SetQuestTitle` 이 제목뿐 아니라 `Objectives.Reset()` 으로 목표 목록 전체를 지운다. 그런데 두 설정 태스크의 생성자는 `bShouldStateChangeOnReselect` 를 기본값(true)으로 두고 있다 — 같은 파일의 `WaitMoveToTarget` 이 이 플래그를 명시적으로 false 로 내리는 것(`WxStateTreeTask_WaitMoveToTarget.cpp:16`)과 대비된다. UE 5.8 `FStateTreeExecutionContext` 는 재선택된(Sustained) 상태의 태스크에 대해 이 플래그가 true 면 ExitState·EnterState 를 **다시** 호출한다(엔진 `StateTreeExecutionContext.cpp:3840`·`4029`). README 가 권장하는 표준 조립(루트 상태에 제목, 자식 스텝 상태에 목표+대기)에서는 스텝 A→B 전이마다 루트가 Sustained 가 되므로 `SetQuestTitle::EnterState` 가 매 스텝 재실행되어 목표 목록을 통째로 비운다. 표준 배치에서는 자식의 EnterState 가 부모보다 뒤라 결과가 우연히 복구되지만, (a) 루트 상태가 스텝을 가로지르는 상시 목표를 들고 있으면 그 목표가 매 전이마다 제거·재등록되어 순서와 핸들이 바뀌고, (b) 한 상태 안에서 `SetQuestObjective` 가 `SetQuestTitle` 보다 앞에 놓이면 EnterState 가 앞에서 뒤로 도는 탓에 방금 등록한 목표가 곧바로 지워진 뒤 핸들만 남아 ExitState 의 `RemoveObjective` 도 노옵이 된다 — 그 목표는 로그 한 줄 없이 저널에서 사라진다. 어느 경우든 스텝 전이마다 `OnJournalChanged` 가 불필요하게 두세 번 발화한다.
- **제안**: 저널 초기화 책임을 태스크에서 걷어내 퀘스트 활성화 쪽(`ClearJournal`)에만 남긴다 — `SetQuestTitle` 은 제목만 갱신하게 하고, 목표 비우기는 이미 러너 정지 경로가 수렴시키고 있으므로 중복이다. 더불어 두 설정 태스크 생성자에 `bShouldStateChangeOnReselect = false` 를 달아 재선택 시 재진입 자체를 막는다(엔진 주석도 "자원을 점유하고 자식 상태에서 유지되는 태스크"는 false 를 권한다 — `SetQuestObjective` 가 정확히 그 유형이다).
- **확신도**: 중간(재진입 메커니즘과 `Objectives.Reset()` 은 확정이나, 실제 손실이 나려면 위 (a)·(b) 같은 조립이 에셋에 실재해야 한다 — ST 에셋은 이번 리뷰 범위 밖이다)

### 2. 🟡 BP 에 열린 `StartQuest` 가 러너 실행 콜스택 안에서 불리면 퀘스트가 조용히 죽는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:34`
- **범주**: 설계/구조
- **문제**: `ActivateQuest` 는 `StopLogic → SetStateTreeReference → StartLogic` 3연타인데, 이 셋 모두 러너 실행 중 호출에 대한 방어가 엔진 쪽에 있고 그 방어가 전부 "로그 남기고 리턴"이다. UE 5.8 `UStateTreeComponent` 기준으로 `StopLogic` 은 `CurrentlyRunningExecContext` 가 있으면 실제 정지를 프레임 끝으로 미루고, `SetStateTreeReference` 는 인스턴스가 Running 이면 경고만 남기고 교체를 거부하며, `StartTree` 는 재진입이면 "Reentrant call" 에러 후 리턴한다. 즉 이 경로로 들어오면 **기존 퀘스트만 정지되고 새 퀘스트는 시작되지 않아** 저널이 빈 채로 시스템이 멈춘다. 컴포넌트 헤더는 "ST 실행 콜스택 밖에서만 호출한다"는 규약을 달아 두었지만, 정작 BP 에 노출된 `UWxQuestLibrary::StartQuest` 는 하드 포인터로 `ActivateQuest` 를 직결하고 라이브러리 헤더에는 그 제약이 적혀 있지 않다. 안전한 짝인 `RequestActivateQuest` 는 BP 에 노출돼 있지 않아, BP 작업자가 고를 수 있는 유일한 진입점이 위험한 쪽이다.
- **제안**: `StartQuest` 도 다음 틱 예약 경로로 보내 두 경우 모두 안전하게 만든다(레벨 트리거에서 부를 때 한 프레임 늦어지는 것 외엔 손해가 없다). 그게 과하면 최소한 라이브러리 헤더에 "퀘스트 StateTree 안에서 부르지 말 것"을 명시한다.
- **확신도**: 중간(호출 지점을 레벨 트리거로 한정하는 현재 규약이 지켜지는 동안에는 발생하지 않는다)

### 3. 🟡 체인 전환마다 다음 퀘스트 에셋을 게임 스레드에서 동기 로드한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:132`
- **범주**: 성능/안전
- **문제**: `HandleDeferredActivateQuest` 가 `LoadSynchronous()` 로 다음 퀘스트 StateTree 를 블로킹 로드한다. 소프트 참조로 넘겨 GC 를 피한 설계 의도는 타당하나, 로드 자체가 동기라 퀘스트 트리와 그 태스크들이 하드 참조하는 것들(README 가 언급하는 GiveRewards 계열이 참조할 보상 아이템·스폰 액터 등)이 전부 딸려 오면 체인이 넘어가는 순간 히치가 난다. 퀘스트 전환은 컷신·대화 직후 같은 눈에 띄는 타이밍에 몰린다.
- **제안**: `UAssetManager::GetStreamableManager().RequestAsyncLoad` 로 비동기 로드 후 완료 콜백에서 `ActivateQuest` 를 호출한다. 지금 구조가 이미 "다음 틱"으로 한 번 미루고 있어 비동기로 바꿔도 호출 규약이 달라지지 않는다.
- **확신도**: 중간(현재 퀘스트 트리들이 가벼우면 체감되지 않을 수 있다 — 실측 후 판단할 여지가 있다)

### 4. 🟢 `StartNextQuest` 의 실패 경로만 완전히 무음이라 끊긴 체인이 흔적을 남기지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:25`
- **범주**: 버그/정확성
- **문제**: 형제 태스크인 `SetQuestTitle`·`SetQuestObjective` 는 같은 상황(오너에서 퀘스트 컴포넌트를 못 찾음)에서 `LogWxQuest` 경고를 남기는데, `StartNextQuest` 만 조용히 `Failed` 를 돌려준다. 그런데 이 태스크는 `bConsideredForCompletion = false` 라서 엔진이 그 `Failed` 를 완료 판정에서 아예 무시한다(UE 5.8 엔진 테스트 `StateTreeTaskStateTest.cpp` 의 `FStateTreeTest_TasksCompletion_IneligibleTaskEnterStateFail` 이 "ineligible EnterState failure is ignored" 로 명시). 결과적으로 잘못 조립된 체인은 로그도 상태 실패도 없이 그냥 다음 퀘스트로 안 넘어간다.
- **제안**: 형제 태스크와 같은 형태로 `UE_LOG(LogWxQuest, Warning, ...)` 한 줄을 맞춰 준다.
- **확신도**: 높음

### 5. 🟢 저널이 체인 전환 사이 한 프레임 비었다가 다시 채워진다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:47`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:122`
- **범주**: 설계/구조
- **문제**: 퀘스트 A 의 트리가 스스로 완료되는 프레임에 실행 상태가 Running 을 벗어나 `ClearJournal` 이 돌고(`bHasActiveQuest=false`, `OnJournalChanged` 발화), 예약된 퀘스트 B 활성화는 다음 틱 타이머라 그 프레임 안에서는 저널이 비어 있다. HUD 뷰모델은 그 발화를 그대로 받아 pull 하므로 퀘스트 패널이 1프레임 사라졌다가 돌아온다. 특히 `StartNextQuest` 만 있는 종단 상태는 완료 판정 대상 태스크가 0개라 진입 즉시 완료되므로(기존에 확인된 5.8 동작) 이 경로가 체인의 표준 형태다.
- **제안**: 기능상 문제는 아니라 방치해도 되지만, 패널에 등장·퇴장 연출이 붙는 순간 깜빡임으로 보인다. 고친다면 저널 정리 자체를 미루기보다 HUD 쪽에서 한 프레임 디바운스하는 편이 컴포넌트에 분기용 상태를 새로 들이지 않아 깔끔하다.
- **확신도**: 중간(연출이 없는 지금은 눈에 안 띌 수 있다)

### 6. 🟢 `GameplayTags` 는 쓰이지 않는 빌드 의존성이다
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`
- **범주**: 중복/복잡도
- **문제**: 모듈 소스 전체에 `FGameplayTag`·태그 관련 사용처가 한 곳도 없다(모듈 내 유일한 "GameplayTag" 문자열이 이 의존성 선언 자신이다).
- **제안**: 줄을 지운다. 나중에 퀘스트 조건을 태그로 표현하게 되면 그때 다시 넣으면 된다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin` — 소비자 쪽 계약 확인용으로 `Source/WxGame/MVVM/WxViewModel_Quest.cpp` 를, 형제 로케이터 태스크와의 대조용으로 `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`·`Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp` 도 읽었다(WxGame 은 게임 모듈이라 WxQuest 참조는 규칙 위반이 아니다).
- **확인했고 문제 없었던 것**:
  - 코딩 규칙 전 항목 통과 — 모든 파일 첫 줄 저작권 표기, `Wx` 접두사, 델리게이트 콜백의 `Handle` 접두사(`HandleStateTreeRunStatusChanged`·`HandleDeferredActivateQuest`), `BlueprintCallable` 은 Blueprint Function Library 인 `UWxQuestLibrary` 한 곳뿐, 람다 사용 없음, 헤더 인라인 정의는 4개 태스크의 `GetInstanceDataType()` 뿐이고 네 파일 모두 예외 사유 주석이 달려 있음.
  - 모듈 경계 — 빌드 의존성·uplugin 모두에 `WxCore` 외 다른 Wx 플러그인이 없다.
  - `bConsideredForCompletion` 을 `#if WITH_EDITORONLY_DATA` 로 감싼 것은 **정상**이다. UE 5.8 `FStateTreeTaskBase` 에서 이 필드 자체가 에디터 전용이고 컴파일러가 상태의 완료 마스크로 구워 넣으므로, 쿠킹 빌드에서 설정 태스크가 완료 판정에 복귀하는 일은 없다.
  - `EnterState` 가 `Failed` 를 반환한 태스크에도 `ExitState` 는 호출된다(엔진 `FStateTreeExecutionContext::ExitState` 는 `EnterState` 가 불린 범위 전체를 역순 순회). 따라서 목표를 걸었다 걷어가는 짝은 실패 경로에서도 새지 않는다. `RemoveObjective(INDEX_NONE)` 도 핸들이 0부터 발급되므로 절대 오매치하지 않는다.
  - `BeginPlay` 의 러너 생성 순서가 맞다 — `SetStartLogicAutomatically(false)` 를 `RegisterComponent()` 앞에 둬서, 이미 BeginPlay 를 마친 오너에 붙는 컴포넌트가 자동 시작되는 것을 막는다. `BeginPlay` 를 public 으로 선언한 것도 `UActorComponent` 의 접근 지정자와 일치한다.
  - `WaitMoveToTarget` 이 `SyncFind(Owner)` 에 GameState 를 컨텍스트로 넘기는 것은 **문제 없다**. 형제 모듈(`WaitForInteraction`)이 GameState 컨텍스트로는 WP 런타임 셀을 못 푼다고 적어 둬서 의심했으나, UE 5.8 `FActorLocatorFragment::Resolve` 는 레벨 경로 실패 시 `FSoftObjectPath::ResolveObject()` 로 폴백하고 그 안에서 `UWorld::ResolveSubobject` 가 WP 를 처리한다. 에디터에서는 컨텍스트 패키지의 PIE 인스턴스 ID 로 경로를 보정해 주므로 오히려 컨텍스트를 넘기는 쪽이 맞다.
  - `NewObject<UStateTreeComponent>(Owner, TEXT("QuestStateTree"))` 의 고정 이름은 중복 생성 시 위험하지만, 주입이 `UGameFrameworkComponentManager::AddComponentRequest` 를 거쳐 (Receiver, Component) 쌍으로 중복 제거되므로 같은 GameState 에 `UWxQuestComponent` 가 두 벌 붙는 경로가 실재하지 않는다.
- **미검토 / 한계**: 저널 미복제(데디케이티드 서버·클라 HUD 가 영구 공백)는 멀티 정책 자체가 보류로 결정된 사안이라 발견으로 올리지 않았다 — 다만 러너를 권위에만 띄우는 `WxQuestComponent.cpp:110` 의 게이팅이 그 결정의 유일한 코드상 근거이므로, 정책이 정해지면 여기가 첫 수정 지점이다. 퀘스트 `UStateTree` 에셋 자체의 조립(상태 구성·태스크 배치 순서·전이 설정)은 범위 밖이며, 발견 1의 실제 영향도는 전적으로 그 조립에 달려 있으므로 함께 확인해야 한다. `GiveRewards` 등 WxQuest 밖 크로스모듈 태스크의 하드 참조 규모도 발견 3의 영향도를 좌우한다.

---
*문서 기준 커밋 `a8c6c495` · 리뷰일 2026-09-01 · 소스 14파일 — `/module-review`로 갱신*
