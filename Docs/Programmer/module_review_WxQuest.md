# WxQuest — 코드 리뷰

> 14파일짜리 작은 모듈이고 권위 모델·에셋 불가지·"상태 수명 = 목표 수명" 규약이 클래스 doc-comment 에 잘 정리돼 있어 전반적으로 건강하다. 남은 위험은 전부 "조용히 어긋나는" 쪽 — StateTree 의 재진입·완료 판정 규약, 그리고 실패 경로가 로그도 종료도 없이 흘러가는 지점들이다. 이번 리뷰는 모듈 소스 14파일(헤더 7 + cpp 7)을 전부 읽고 핵심 로직(`WxQuestComponent.cpp`, 태스크 4종 cpp)을 깊게 봤으며, 지난 리뷰가 확인하지 못한 StateTree 엔진 측 전제를 UE 5.8 설치 경로의 엔진 소스·자동화 테스트로 직접 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 저널 태스크가 Sustained 재진입에 무방비 — 부모 상태에 걸면 자식 전이마다 목표가 뽑혔다 다시 꽂힌다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:10-18`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:10-18`
- **범주**: 버그/정확성
- **문제**: 두 태스크의 생성자는 `bShouldCallTick` 과 완료 판정 플래그만 건드리고 `bShouldStateChangeOnReselect` 를 기본값(`true`)으로 둔다. 엔진 실행 컨텍스트는 `ChangeType == Sustained`(부모 상태가 계속 활성인 채 자식만 바뀌는 전이)일 때 이 플래그가 참인 태스크에만 `EnterState`/`ExitState` 를 다시 부른다 — UE 5.8 `StateTreeExecutionContext.cpp` 의 `bShouldCallEnterState`·`bShouldCallStateChange` 계산에서 확인된다. 그래서 "목표는 부모 상태, 스텝은 자식 상태"라는 자연스러운 조립에서 자식 전이 한 번마다 `RemoveObjective` → `AddObjective` 가 돌아 핸들이 새로 발급되고 `OnJournalChanged` 가 전이당 여러 번 튄다. 구독자는 브로드캐스트마다 목표 뷰모델 UObject 를 통째로 재할당한다(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:51-62`). `SetQuestTitle` 은 더해서 재진입 시 `Objectives.Reset()`(`Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:53`)까지 다시 돌려, 아직 살아 있는 자식 상태의 목표를 저널에서 지워 버린다 — 그 목표는 자기 상태가 끝날 때까지 다시 나타나지 않는다. `WxStateTreeTask_SetQuestObjective.h:28` 이 선언한 "상태에 머무는 동안 유지" 계약과 정면으로 어긋난다. 엔진 `FStateTreeTaskBase::bShouldStateChangeOnReselect` 의 doc-comment 자체가 "자식 상태에서 획득될 것으로 기대되는 리소스 점유형 태스크는 false 여야 한다"고 못 박고 있고, 같은 모듈의 `WxStateTreeTask_WaitMoveToTarget.cpp:16` 은 이 플래그를 명시적으로 꺼놨으므로 저널 태스크만 빠진 것으로 보인다.
- **제안**: 두 태스크 생성자에 `bShouldStateChangeOnReselect = false;` 를 추가한다.
- **확신도**: 높음

### 2. 🟡 완료 판정에서 뺀 태스크의 `Failed` 반환은 엔진이 무시한다 — 헤더 문서가 약속한 실패 종료가 일어나지 않고, `StartNextQuest` 는 로그조차 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:23-26`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:24-29`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:24-29`
- **범주**: 버그/정확성
- **문제**: 세 태스크는 생성자에서 `bConsideredForCompletion = false` 로 완료 판정에서 스스로 빠져 놓고, 퀘스트 컴포넌트를 못 찾으면 `EStateTreeRunStatus::Failed` 를 돌려준다. 지난 리뷰가 미확인으로 남긴 이 전제를 이번에 엔진 자동화 테스트로 확정했다 — UE 5.8 StateTree 테스트 스위트의 `FStateTreeTest_TasksCompletion_IneligibleTaskEnterStateFail` 은 "완료 판정에서 빠진 태스크가 `EnterState` 에서 Failed 를 돌려줘도 상태 진입을 막아서는 안 된다"를 명시적으로 단언한다(그 태스크는 이후 Tick 도 받지 않을 뿐이다). 따라서 헤더 문서(`WxStateTreeTask_StartNextQuest.h:27` "예약 없이 Failed 로 끝난다", `WxStateTreeTask_SetQuestTitle.h:28`, `WxStateTreeTask_SetQuestObjective.h:32`)가 약속한 실패 종료는 실제로 존재하지 않고, 트리는 잘못 조립된 채로 계속 굴러간다. 제목·목표 태스크는 그래도 `LogWxQuest` 경고를 남기지만 `StartNextQuest` 는 조기 반환에 로그가 하나도 없어 퀘스트 체인이 아무 흔적 없이 끊긴다.
- **제안**: `StartNextQuest` 의 조기 반환에 형제 태스크와 같은 `LogWxQuest` 경고를 넣는다. 그리고 세 헤더의 "Failed 로 끝난다" 서술을 실제 동작(경고만 남기고 상태는 계속 진행)대로 정정한다 — 실패를 상태에 실제로 전파하고 싶다면 `bConsideredForCompletion = false` 전제를 포기해야 하는데, 그러면 Set 계열이 진입 즉시 상태를 끝내 버리므로 현재 설계와 양립하지 않는다.
- **확신도**: 높음

### 3. 🟡 `WaitMoveToTarget` 은 대상 해석에 실패해도 영원히 Running — 퀘스트가 조용히 멈춘다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23-28`, `:48-54`
- **범주**: 버그/정확성
- **문제**: 빈 로케이터는 진입 시 경고 한 줄을 남기고도 `Running` 을 반환해(`:25-28`) 완료될 수 없는 상태에 그대로 머무른다. 대상이 해석되지 않는 경우(`SyncFind` 가 null — 액터 삭제·WP 셀 미로드·PIE 경로 불일치)도 `:48-52` 에서 아무 로그 없이 Running 을 이어간다. 헤더(`WxStateTreeTask_WaitMoveToTarget.h:34`)가 밝혔듯 `SyncFind` 는 강제 로드를 하지 않으므로 미로드 셀의 액터는 정상적으로 null 이 나온다. 상태 완료를 내는 유일한 태스크가 이것이므로 결과는 "퀘스트가 그 스텝에서 영구 정지"이고, 로그가 없어 원인 추적도 어렵다. 발견 2 와 달리 이 태스크는 완료 판정 대상이므로 `Failed` 반환이 실제로 상태에 전파된다 — 즉 고칠 수단이 있다.
- **제안**: 빈 로케이터는 `EnterState` 에서 `Failed` 로 끝낸다. 해석 실패는 최소 1회성 경고를 남기거나, 허용 시간을 넘기면 실패 처리한다.
- **확신도**: 중간(빈 로케이터를 경고만 하고 넘기는 것은 `WxStateTreeTask_WaitMoveToTarget.h:31` 에 명시된 현재 의도이므로 방침 변경에 해당한다)

### 4. 🟡 `ActivateQuest` 가 에셋 교체 성공을 확인하지 않고, 그 경로를 BP 에 무방비로 노출한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:32-36`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h:23-24`
- **범주**: 설계/구조
- **문제**: `StopLogic` → `SetStateTreeReference` → `StartLogic` 세 호출 중 어느 것도 결과를 보지 않는다. 엔진 `UStateTreeComponent` 소스로 실패 연쇄를 확인했다 — 러너 실행 콜스택 안에서 `StopLogic` 이 불리면 정지가 프레임 끝으로 미뤄지고, `SetStateTreeReference` 는 실행 상태가 아직 `Running` 이면 "Trying to change the state tree on a running instance." 경고만 남기고 교체를 거부하며, 이어지는 `StartLogic` 도 "Reentrant call to StartTree is not allowed." 로 반려된다. 결과는 크래시가 아니라 "새 퀘스트가 조용히 사라지고 저널만 비는" 상태다. 헤더 주석(`WxQuestComponent.h:53`)은 "ST 실행 콜스택 밖에서만 호출"을 못 박았지만, 실제 저작 진입점인 `UWxQuestLibrary::StartQuest` 는 이 무방비 경로를 BlueprintCallable 로 그대로 노출한다 — 안전한 짝인 `RequestActivateQuest`(`WxQuestComponent.h:56-57`)는 BP 에서 보이지 않으므로 저작자가 규약을 어길 수단만 있고 지킬 수단이 없다.
- **제안**: `ActivateQuest` 에서 교체 후 러너가 새 에셋을 들고 있는지(또는 `IsRunning()` 이 여전히 참인지) 확인해 실패를 `LogWxQuest` 로 남긴다. 더 나아가려면 실행 중이면 `RequestActivateQuest` 의 다음 틱 경로로 흘려보내 진입점 하나로 수렴시킨다.
- **확신도**: 중간(현재 확인되는 호출자는 `Content/Quest/BP_QuestVolume` 계열 트리거 볼륨과 `StartNextQuest` 의 안전 경로뿐이라 실제로 재현되지 않을 수 있음)

### 5. 🟡 `bHasActiveQuest` 가 "퀘스트 활성"과 "저널에 제목이 있음"을 겸업한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:100`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:54`, `:137-140`
- **범주**: 설계/구조
- **문제**: 이 플래그는 오직 `SetQuestTitle` 에서만 참이 된다(`WxQuestComponent.cpp:54`). 그래서 제목 태스크를 쓰지 않거나 제목을 뒤쪽 상태에서 거는 퀘스트는, 러너가 멀쩡히 돌고 목표까지 저널에 올라와 있는데도 `HasActiveQuest()` 가 false 다. 이 값을 HUD 표시 조건으로 쓰는 구독자(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:46`)에선 "목표는 있는데 저널이 안 뜨는" 형태로 드러난다. 게다가 `ClearJournal` 이 이 플래그로 조기 반환하므로(`WxQuestComponent.cpp:137-140`) 제목 없이 목표만 있던 저널은 종료 시 정리 통지가 나가지 않는다. 진짜 "활성" 상태는 러너(`UStateTreeComponent::IsRunning()`)가 갖고 있고, 저널 유무는 `QuestTitle`/`Objectives` 에서 그대로 파생된다 — 별도 플래그가 세 번째 진실 원본이 되어 있다.
- **제안**: 플래그를 없애고 `HasActiveQuest()` 를 러너 상태에서, 저널 유무는 `QuestTitle`/`Objectives` 에서 파생시킨다. 최소한 `ClearJournal` 의 조기 반환 조건만이라도 "제목 또는 목표가 하나라도 있으면"으로 넓힌다.
- **확신도**: 중간(모든 퀘스트가 제목 태스크로 시작한다는 저작 관례가 전제라면 의도된 설계일 수 있음)

### 6. 🟢 미사용 빌드 의존성 `GameplayTags`
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:16`
- **범주**: 중복/복잡도
- **문제**: 모듈 소스 전체에 게임플레이 태그 사용이 0건인데 Public 의존성으로 선언돼 있다. 같은 커밋대에서 `ModularGameplay` 는 정리됐는데 이건 남았다.
- **제안**: 제거한다.
- **확신도**: 높음

### 7. 🟢 다음 틱 예약 경로의 무검사 `GetWorld()` 와 게임 스레드 동기 로드
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:47`, `:132`
- **범주**: 성능/안전
- **문제**: `GetWorld()->GetTimerManager()` 가 반환값을 검사하지 않고 역참조한다(플레이 중엔 유효하지만 계약상 무방비). 이어지는 `HandleDeferredActivateQuest` 는 `LoadSynchronous()` 로 퀘스트 에셋과 그 하드 참조를 게임 스레드에서 동기 로드하므로 체인 전환 프레임에 히치가 생길 여지가 있다.
- **제안**: `GetWorld()` 널 검사를 추가하고, 히치가 관측되면 `FStreamableManager` 비동기 요청으로 바꾼다.
- **확신도**: 중간(에셋이 작으면 체감되지 않을 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`
- **훑은 파일**: 태스크 4종 헤더, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`. 참고용(리뷰 대상 아님)으로 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`(구독자 영향), `Source/WxGame/Framework/WxGameState.cpp`(부착 경로), 그리고 UE 5.8 엔진 소스의 `StateTreeExecutionContext.cpp`·`StateTreeTaskBase.h`·`StateTreeCompiler.cpp`·`StateTreeComponent.cpp`·StateTree 테스트 스위트를 확인했다.
- **규칙 점검 결과**: `CLAUDE.md` 코딩·모듈 규칙 위반은 발견되지 않았다. 저작권 첫 줄 15/15(Build.cs 포함) 준수, `Wx` prefix 준수, 델리게이트·타이머 콜백 `Handle` prefix 준수(`HandleStateTreeRunStatusChanged`, `HandleDeferredActivateQuest`), `BlueprintCallable` 은 `UWxQuestLibrary::StartQuest` 1건뿐이며 BP Function Library 소속, 헤더 인라인 정의는 `GetInstanceDataType()` 4건이고 전부 예외 사유 주석이 붙어 있으며, 람다·`FORCEINLINE` 사용 0건, 의존 플러그인은 `WxCore` 와 엔진 플러그인뿐이다(`ModularGameplay` 는 제거됨).
- **미검토 / 한계**: 퀘스트 `UStateTree` 에셋(`Content/Quest/ST_Quest_Main1`·`Main2`, `Content/Quest/Steps/ST_QuestStep_*`)의 실제 상태 구성은 BP/에셋 영역이라 열지 않았으므로, 발견 1 이 현재 저작물에서 실제로 터지는지(목표를 부모 상태에 거는 조립이 쓰이는지)는 미정이다. 저널의 리플리케이션 부재는 클래스 문서에 명시된 v1 유보 사항이라 발견으로 잡지 않았다. 지난 리뷰의 "런타임 생성 러너의 수명 정리 부재"는 이번에 뺐다 — Experience 주입이 제거되어 컴포넌트가 `AWxGameState` 생성자 기본 서브오브젝트가 되면서 러너와 수명이 묶였고, `UStateTreeComponent::EndPlay` 가 스스로 `StopLogic` 을 부르므로 유령 러너 시나리오가 성립하지 않는다. 같은 이유로 "컴포넌트 조회 2줄의 4중 복제"도 발견에서 뺐다 — 프로젝트가 작은 헬퍼 추출보다 인플레이스 반복을 선호하는 방침이라, 실제로 고쳐야 할 부분(실패 처리 불일치)은 발견 2 에 흡수했다.

---
*문서 기준 커밋 `53bc7de6` · 리뷰일 2026-09-08 · 소스 14파일 — `/module-review`로 갱신*
