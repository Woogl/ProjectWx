# WxQuest — 코드 리뷰

> 14파일짜리 작은 모듈이고 권위 모델·에셋 불가지·"목표 수명 = 상태 수명" 규약이 doc-comment 에 정리돼 있어 구조는 건강하다. 크래시성 결함·수명주기 위험·모듈 경계 침범은 없고 `CLAUDE.md` 규칙도 전부 지킨다(저작권 첫 줄 15/15, `Wx` prefix·`Handle` prefix 준수, 헤더 인라인은 예외 주석이 붙은 `GetInstanceDataType()` 4건뿐, 람다 0건, `BlueprintCallable` 은 BP Function Library 1건, 의존은 `WxCore` + 엔진 모듈뿐). 남은 위험은 전부 StateTree 의 재진입·완료 판정 규약을 전제만 하고 확인하지 않아 "조용히 어긋나는" 지점이다. 이번 리뷰는 모듈 소스 14파일을 모두 읽고 컴포넌트와 태스크 4종 cpp 를 깊게 봤으며, 판정 근거인 엔진 동작은 UE 5.8 설치본의 `StateTreeComponent.cpp`·`StateTreeExecutionContext.cpp`·`StateTreeTasksStatus.h/.cpp`·`StateTreeCompiler.cpp`·`StateTreeTaskBase.h`·`TimerManager.cpp` 에서 직접 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `ActivateQuest` 가 러너 콜스택 안 호출을 막지 못해, BP 경로에서 새 퀘스트가 조용히 버려진다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:18-37`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h:23-24`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:74-76`
- **범주**: 버그/정확성
- **문제**: `StopLogic` → `SetStateTreeReference` → `StartLogic`(`:34-36`) 중 어느 결과도 확인하지 않는다. 러너 실행 콜스택 안에서 불리면 엔진에서 다음 사슬이 확인된다. `UStateTreeComponent::StopLogic` 은 `bIsRunning` 을 먼저 내리고(`StateTreeComponent.cpp:221`) 기존 컨텍스트로 `Stop` 을 부르는데, 업데이트 단계 중이면 `FStateTreeExecutionContext::Stop` 이 정지를 프레임 끝으로 미루고 `Running` 을 돌려준다(`StateTreeExecutionContext.cpp:1707-1715`). 이어서 `SetStateTreeReference` 는 실행 상태가 아직 Running 이라 경고만 남기고 교체를 거부하고(`StateTreeComponent.cpp:493-501`), `StartTree` 는 재진입 가드에 반려된다(`:181-185`). 결과는 지연 정지가 기존 퀘스트를 끝내고 저널을 비우는 동안 새 퀘스트는 시작조차 되지 않는 상태다. 이 경로는 BP 에 열려 있다 — BlueprintAssignable `OnJournalChanged`(`WxQuestComponent.h:74-76`)는 태스크 Enter/Exit 과 러너 실행 상태 통지 안에서 발화하므로 여기에 반응해 `UWxQuestLibrary::StartQuest` 를 부르는 BP 는 재진입이 되고, 퀘스트 ST 안의 BP 태스크도 마찬가지다. 안전한 짝인 `RequestActivateQuest`(`WxQuestComponent.h:56-57`)는 UFUNCTION 이 아니라 BP 에서 쓸 수 없고, `:20-29` 의 조기 반환은 널 에셋·비권위·BeginPlay 전 세 경우를 구분 없이 침묵으로 삼킨다.
- **제안**: `StopLogic` 뒤 `QuestStateTree->GetStateTreeRunStatus()` 가 여전히 `Running` 이면 `LogWxQuest` 에러를 남기고 중단한다. 근본적으로는 `UWxQuestLibrary::StartQuest` 도 다음 틱 경로(`RequestActivateQuest`)로 보내 활성화 진입점을 하나로 수렴시킨다.
- **확신도**: 중간(현재 C++ 호출자는 `StartQuest` 와 다음 틱 경로뿐이라 재진입은 BP 저작으로만 열린다)

### 2. 🟡 저널 태스크 3종이 `bShouldStateChangeOnReselect` 기본값(true)을 그대로 둬 Sustained 재진입 시 저널이 어긋난다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:10-18`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:10-18`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:9-17`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:53`
- **범주**: 버그/정확성
- **문제**: 세 생성자는 `bShouldStateChangeOnReselect` 를 엔진 기본값 `true`(`StateTreeTaskBase.h:24`)로 남긴다. UE 5.8 에서 Sustained 는 이미 활성인 상태가 전이 대상이 되거나 그 아래에서 다시 선택될 때만 생기고(대상이 아닌 공통 조상은 건너뛴다 — `StateTreeExecutionContext.cpp:3711-3723`), 이때 이 플래그가 참인 태스크만 Exit/Enter 가 다시 불린다(`:3839-3840`, `:4028-4029`). 따라서 형제 스텝으로 가는 일반 전이에는 영향이 없지만, 활성 조상이나 자기 자신을 대상으로 하는 전이(스텝 재시도용 "부모로 가기", 이벤트로 자기 상태 재선택 등)가 저작되면 두 가지가 어긋난다. (a) 대상 상태의 제목 태스크가 재진입해 `SetQuestTitle` 의 `Objectives.Reset()`(`WxQuestComponent.cpp:53`)을 다시 돈다. Enter 는 상위→하위, 상태 안에서는 선언 순으로 돌기 때문에(`:3816`) 같은 상태에서 목표 태스크가 제목보다 먼저 선언돼 있거나 목표가 대상보다 위 조상 상태에 걸려 있으면(조상은 Enter 가 다시 불리지 않는다) 그 목표가 지워지고 상태가 끝날 때까지 복구되지 않는다 — `WxStateTreeTask_SetQuestObjective.h:28` 의 "상태에 머무는 동안 유지" 계약 위반이다. 그 외 경우에도 핸들 재발급과 `OnJournalChanged` 다중 발화가 남는다. (b) `StartNextQuest` 가 재진입하면 다음 틱 예약이 한 번 더 쌓여 발견 6 의 이중 활성화로 이어진다. 엔진 doc-comment(`StateTreeTaskBase.h:107-112`)는 자식 상태 동안 유지되는 리소스형 태스크는 false 여야 한다고 명시하며, 같은 모듈의 `WxStateTreeTask_WaitMoveToTarget.cpp:16` 은 이미 false 로 둔다.
- **제안**: 세 태스크 생성자에 `bShouldStateChangeOnReselect = false;` 를 추가한다.
- **확신도**: 중간(엔진 동작은 소스로 확인했으나, 현재 퀘스트 에셋에 활성 상태를 대상으로 하는 전이가 있는지는 미확인이라 잠재 결함일 수 있다)

### 3. 🟡 `WaitMoveToTarget` 은 빈 로케이터나 사라진 대상에도 영원히 Running — 퀘스트가 흔적 없이 멈춘다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23-28`, `:48-54`
- **범주**: 버그/정확성
- **문제**: 빈 로케이터는 진입 시 경고 한 줄만 남기고 `Running` 을 반환해(`:23-28`) 완료될 수 없는 상태에 머문다. 대상이 해석되지 않는 경우도 `:48-54` 에서 로그 없이 Running 을 이어간다. WP 셀 미로드는 플레이어가 다가가면 풀리므로 Running 이 맞지만(`SyncFind` 는 강제 로드하지 않는다 — `WxStateTreeTask_WaitMoveToTarget.h:33`), 액터가 삭제·이름 변경돼 로케이터가 영구히 깨진 경우도 똑같이 무기한 대기한다. 이 태스크는 완료 판정에 참여하므로 `Failed` 반환이 상태에 실제로 전파되는데도 그 수단을 쓰지 않는다. 스텝 상태의 완료를 내는 태스크가 이것이라 결과는 "그 스텝에서 영구 정지"이고 원인 로그도 없다.
- **제안**: 빈 로케이터는 `EnterState` 에서 `Failed` 로 끝낸다. 해석 실패는 스트리밍을 고려해 Running 을 유지하되, 일정 시간 이상 지속되면 대상 경로를 담은 1회성 경고를 남긴다.
- **확신도**: 중간(빈 로케이터를 경고만 하고 넘기는 것은 `WxStateTreeTask_WaitMoveToTarget.h:30` 에 적힌 현재 의도라 방침 변경에 해당)

### 4. 🟡 `bHasActiveQuest` 가 "퀘스트 활성"이 아니라 "제목 등록됨"을 뜻한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:100`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:54`, `:83-86`, `:137-140`
- **범주**: 설계/구조
- **문제**: 이 플래그가 참이 되는 곳은 `SetQuestTitle`(`:54`)뿐이다. 제목 태스크 없이 목표만 거는 퀘스트나 제목을 뒤쪽 상태에서 거는 퀘스트는 러너가 돌고 목표가 저널에 올라와 있어도 `HasActiveQuest()` 가 false 이고, 이 값을 표시 조건으로 쓰는 구독자(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:53`)에선 "목표는 있는데 HUD 가 숨는" 형태로 드러난다. 저널 내용이나 러너 상태에서 조회할 수 있는 값을 별도 플래그로 저장해 동기화 책임만 늘린 형태다.
- **제안**: 플래그를 없애고 `QuestTitle`/`Objectives` 유무, 또는 `QuestStateTree` 가 있고 `GetStateTreeRunStatus() == EStateTreeRunStatus::Running` 인지에서 파생시킨다(`ClearJournal` 의 조기 반환도 같은 기준으로). 단 `IsRunning()` 은 쓰면 안 된다 — `UStateTreeComponent` 는 트리가 스스로 완료돼도 `bIsRunning` 을 내리지 않는다(갱신 지점은 `StateTreeComponent.cpp:126`, `:176`, `:201`, `:221`, `:241` 뿐이고 `TickComponent` 의 완료 경로엔 없다).
- **확신도**: 중간(모든 퀘스트가 제목 태스크부터 건다는 저작 관례가 전제라면 의도된 설계일 수 있음)

### 5. 🟡 퀘스트 체인이 끊기는 두 실패 경로가 로그를 남기지 않고, 헤더가 약속한 `Failed` 는 엔진이 무시한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:23-26`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:130-133`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h:27`
- **범주**: 버그/정확성
- **문제**: Set 계열 3종은 `bConsideredForCompletion = false` 라 `EnterState` 반환값이 상태 결과에 반영되지 않는다(`StateTreeExecutionContext.cpp:3873-3880` 은 완료 판정 참여 태스크만 `Result` 에 합친다). 그래서 `WxStateTreeTask_StartNextQuest.h:27` 의 "예약 없이 Failed 로 끝난다"는 실제로는 "아무 일 없이 상태가 계속된다"이다. 제목·목표 태스크는 같은 상황에서 `LogWxQuest` 경고를 남기지만 `StartNextQuest` 의 조기 반환(`:23-26`)은 로그가 없다. 예약 이후도 같다 — `HandleDeferredActivateQuest` 의 `LoadSynchronous()`(`WxQuestComponent.cpp:132`)가 에셋 삭제·이동으로 null 을 돌려주면 `ActivateQuest` 의 널 검사(`:20-23`)에서 조용히 반환한다. 어느 쪽이든 체인이 끊겨도 로그 한 줄 남지 않는다.
- **제안**: `StartNextQuest` 조기 반환과 `HandleDeferredActivateQuest` 의 로드 실패에 `LogWxQuest` 경고(오너 이름·소프트 참조 경로 포함)를 넣고, 헤더 서술을 실제 동작(경고만 남기고 상태는 계속)대로 고친다. 실패를 상태에 전파하려고 `bConsideredForCompletion` 을 켜면 진입 즉시 `Succeeded` 가 상태를 끝내 버려 현재 설계와 양립하지 않는다.
- **확신도**: 높음

### 6. 🟢 `RequestActivateQuest` 의 다음 틱 예약이 누적되고 취소할 수 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:39-48`
- **범주**: 버그/정확성
- **문제**: `SetTimerForNextTick` 은 호출마다 새 `FTimerData` 를 만들 뿐 중복 제거가 없고(`TimerManager.cpp:736-749`), 반환 핸들을 버리므로 앞선 예약을 취소·대체할 수 없다. 예약부터 실행까지의 한 프레임 창에 요청이 겹치면 전부 순서대로 실행된다. 발견 2 의 재진입으로 같은 요청이 두 번 쌓이면 다음 틱에 새 퀘스트가 시작 직후 한 번 더 재시작되고, 예약 직후 트리거 볼륨이 `StartQuest` 로 다른 퀘스트를 즉시 시작하면 뒤늦은 예약이 그 퀘스트를 덮는다. 어느 경우든 덮이는 쪽의 진입 상태 부수효과는 이미 실행된 뒤이며, 퀘스트 ST 에는 `FWxStateTreeTask_GiveRewards`(WxInventory)·`FWxStateTreeTask_TriggerSpawners`(WxWorld) 같은 되돌릴 수 없는 노드가 붙는다.
- **제안**: 예약 `FTimerHandle` 을 멤버로 두고, 새 예약 전과 `ActivateQuest` 즉시 경로에서 기존 예약을 `ClearTimer` 해 마지막 요청만 유효하게 한다.
- **확신도**: 중간(누적·취소 불가는 소스로 확인했고, 실제 충돌은 한 프레임 창에 요청이 겹칠 때만 드러난다)

### 7. 🟢 "완료 판정에서 빠져 있어 상태를 끝내지 않는다" doc-comment 에 전제가 빠졌다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h:26`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h:30`
- **범주**: 버그/정확성
- **문제**: 태스크 자신의 기여만 보면 맞지만, 상태에 완료 판정 참여 태스크가 하나도 없으면 성립하지 않는다. UE 5.8 컴파일러는 그런 상태에 "상태 자신" 비트를 태스크 비트 뒤(오프셋 = 태스크 수)에 붙이는데(`StateTreeCompiler.cpp:390-400`), 상태 진입 시 `ResetStatus(TasksNum)` 은 태스크 수만큼만 비트를 지운다(`StateTreeTasksStatus.cpp:177-181`, `StateTreeTasksStatus.h:245-265`). 같은 부모의 형제 상태들은 시작 비트를 공유하므로(`StateTreeCompiler.cpp:1353-1357`), 직전 형제가 그 위치에 남긴 완료 비트 — 비참여 태스크의 `Succeeded` 반환도 비트 자체는 기록한다(`StateTreeTasksStatus.h:212-226`) — 를 물려받아 예컨대 목표 태스크만 얹은 스텝 상태가 진입 직후 완료로 판정된다(`StateTreeExecutionContext.cpp:4876-4881`). 새 퀘스트 상태를 짜는 사람이 이 문장을 근거로 대기 태스크 없는 상태를 만들면 그대로 함정에 빠진다.
- **제안**: 두 문장에 "단, 그 상태에 완료 판정 참여 태스크(대기 계열)가 최소 하나 있어야 한다"는 전제를 덧붙인다.
- **확신도**: 중간(엔진 메커니즘은 소스로 확인했으나, 현재 퀘스트 에셋에 판정 참여 태스크 없는 상태가 있는지는 미확인)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`
- **훑은 파일**: 태스크 4종 헤더(`Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_*.h`), `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`. 참고용(리뷰 대상 아님)으로 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`(구독자 영향), 저장소 전체의 `UWxQuestComponent`·`ActivateQuest` 호출처(`Source/WxGame/Framework/WxGameState.cpp` 부착 1곳뿐), UE 5.8 엔진의 `StateTreeComponent.cpp`·`StateTreeExecutionContext.cpp`(Stop·EnterState·ExitState·틱 완료 집계)·`StateTreeTasksStatus.h/.cpp`·`StateTreeCompiler.cpp`(완료 마스크 생성)·`StateTreeTaskBase.h`·`TimerManager.cpp` 를 확인했다.
- **미검토 / 한계**: 퀘스트 `UStateTree` 에셋(`Content/Quest/ST_Quest_Main1`·`Main2`, `Content/Quest/Steps/ST_QuestStep_*`)과 `BP_QuestVolume` 은 에셋/BP 영역이라 열지 않았으므로, 발견 2·6·7 이 현재 저작물에서 실제로 성립하는지는 미정이다. 같은 에셋으로 `StartQuest` 를 다시 부르면 진행 중인 퀘스트가 처음부터 재시작(진입 부수효과 재실행)되는데 C++ 에는 이를 막는 장치가 없고 호출 측 BP 의 1회성 처리에 기대는 구조라 발견으로 잡지 않았다. 저널 리플리케이션 부재는 `WxQuestComponent.h:36` 에 명시된 v1(싱글/리슨 호스트) 유보라 발견에서 뺐다 — 데디케이티드 서버로 가면 `WaitMoveToTarget` 의 0번 컨트롤러 전제(`WxStateTreeTask_WaitMoveToTarget.cpp:41`)와 함께 재설계 대상이다. 퀘스트 진행도 영속화는 존재하지 않는 기능이라 발견에서 뺐다. 지난 리뷰(`04420d246`) 이후 모듈 소스 변경은 주석뿐이며 발견은 모두 현재 코드로 다시 검증했다. 그 과정에서 지난 판 발견 2 의 "스텝이 넘어갈 때마다 제목 태스크가 재진입한다"는 서술은 UE 5.8 선택 로직(`StateTreeExecutionContext.cpp:3717-3723`, 대상이 아닌 공통 조상은 재진입하지 않음)과 맞지 않아 트리거 조건을 "활성 상태를 대상으로 하는 전이"로 좁혀 정정했고, 지난 판의 무검사 `GetWorld()`·게임 스레드 동기 로드 발견은 신호가 낮아 빼고 로드 실패 침묵만 발견 5 에 합쳤다.

---
*문서 기준 커밋 `9d8cb2dd` · 리뷰일 2026-09-14 · 소스 14파일 — `/module-review`로 갱신*
