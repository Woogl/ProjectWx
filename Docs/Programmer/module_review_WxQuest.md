# WxQuest — 코드 리뷰

> 14파일짜리 작은 모듈로, 권위 모델·에셋 불가지·"목표 수명 = 상태 수명" 규약이 doc-comment 에 정리돼 있어 구조는 건강하다. 크래시성 결함·수명주기 위험·모듈 경계 침범은 없고 의존도 `WxCore` 와 엔진 모듈뿐이며, 남은 위험은 StateTree 재진입·완료 판정 규약과 수주 시점을 전제로만 두고 확인하지 않아 "로그 없이 어긋나는" 지점들이다. 이번 리뷰는 모듈 소스 14파일을 모두 읽고 컴포넌트와 태스크 4종 cpp 를 깊게 봤으며, 판정 근거인 엔진 동작은 UE 5.8 설치본 소스에서 직접 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 즉시 수주 경로 `ActivateQuest` 가 호출 시점 전제를 확인하지 않아 새 퀘스트가 조용히 버려진다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:18-37`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:105-120`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:10-18`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:74-76`
- **범주**: 버그/정확성
- **문제**: BP 유일 진입점 `UWxQuestLibrary::StartQuest` 는 `ActivateQuest` 를 곧장 부르는데, `ActivateQuest` 는 두 전제를 확인하지 않고 어긋나면 로그 없이 끝난다. (a) 러너 콜스택 밖이어야 한다(`WxQuestComponent.h:53`). 콜스택 안에서 불리면 `UStateTreeComponent::StopLogic` 이 `bIsRunning` 을 먼저 내리고(`StateTreeComponent.cpp:221`), 업데이트 단계 중인 `FStateTreeExecutionContext::Stop` 은 정지를 미루고 `Running` 을 돌려주며(`StateTreeExecutionContext.cpp:1707-1715`), `SetStateTreeReference` 는 트리가 아직 Running 이라 교체를 거부하고(`StateTreeComponent.cpp:493-501`), `StartTree` 는 재진입 가드에 막힌다(`:181-185`). 결과는 미뤄 둔 정지(`StateTreeExecutionContext.cpp:1801-1806`)가 기존 퀘스트를 끝내고 저널을 비우는 동안 새 퀘스트는 시작되지 않는 상태다. BlueprintAssignable `OnJournalChanged`(`WxQuestComponent.h:74-76`)는 태스크 Enter/Exit 과 실행 상태 통지 안에서 발화하므로 여기에 반응해 `StartQuest` 를 부르는 BP 가 이 경로를 타고, 퀘스트 ST 안의 BP 태스크도 마찬가지다. (b) 권위 측 `BeginPlay` 이후여야 한다. 러너는 `BeginPlay`(`WxQuestComponent.cpp:105-120`)에서야 생기고, 그 전 호출은 `:26-29` 에서 비권위 호출과 똑같이 조용히 반환된다. GameState 는 `AGameModeBase::PreInitializeComponents` 에서 런타임 스폰되어 퍼시스턴트 레벨 액터 목록 끝에 붙고(`GameModeBase.cpp:132`, `LevelActor.cpp:739`), 에디터 빌드의 `AWorldSettings::NotifyBeginPlay` 는 퍼시스턴트 레벨 목록 순서대로 BeginPlay 를 돈다(`WorldSettings.cpp:370-375`, `EngineUtils.h:220`). 따라서 PIE 에서는 레벨 BP 나 퍼시스턴트 레벨 배치 액터의 BeginPlay 에서 첫 퀘스트를 거는 저작이 매번 조용히 실패하고, 쿠킹 빌드에서는 이 순서가 정해져 있지 않다(`EngineUtils.h:256-258`). 초기 오버랩도 각 액터 BeginPlay 끝에서 계산되므로(`Actor.cpp:4801`) 퍼시스턴트 레벨의 수주 볼륨이 시작부터 폰과 겹쳐 있으면 같은 창에 들 수 있다.
- **제안**: `UWxQuestLibrary::StartQuest` 도 다음 틱 경로(`RequestActivateQuest`)로 보내 활성화 진입점을 하나로 모은다. 다음 틱이면 러너 콜스택 밖이고 월드 BeginPlay 도 끝나 있어 (a)(b)가 함께 풀린다(예약 누적인 발견 6 과 같이 처리). 별도로 `ActivateQuest` 는 `StopLogic` 뒤에도 `QuestStateTree->GetStateTreeRunStatus()` 가 `Running` 이거나, 권위인데 러너가 없으면 `LogWxQuest` 경고를 남기고 중단한다.
- **확신도**: 중간(엔진 동작은 소스로 확인했다. 현재 `StartQuest` 를 부르는 에셋은 `Content/Quest/BP_QuestVolume` 하나뿐이라 실제 발생 여부는 그 BP 와 맵 배치에 달려 있다)

### 2. 🟡 저널 태스크 3종이 `bShouldStateChangeOnReselect` 기본값(true)을 그대로 둬 Sustained 재진입 시 저널이 어긋난다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:10-18`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:10-18`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:9-17`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:53`
- **범주**: 버그/정확성
- **문제**: 세 생성자는 `bShouldStateChangeOnReselect` 를 엔진 기본값 `true`(`StateTreeTaskBase.h:24`)로 남긴다. UE 5.8 에서 Sustained 는 이미 활성인 상태가 전이 대상이 되거나 그 아래에서 다시 선택될 때만 생기고(대상이 아닌 공통 조상은 건너뛴다 — `StateTreeExecutionContext.cpp:3711-3723`), 이때 이 플래그가 참인 태스크만 Exit/Enter 가 다시 불린다(`:3839-3840`, `:4028-4029`). 따라서 형제 스텝으로 가는 일반 전이에는 영향이 없지만, 활성 조상이나 자기 자신을 대상으로 하는 전이(스텝 재시도용 "부모로 가기", 이벤트로 자기 상태 재선택 등)가 저작되면 두 가지가 어긋난다. (a) 대상 상태의 제목 태스크가 재진입해 `SetQuestTitle` 의 `Objectives.Reset()`(`WxQuestComponent.cpp:53`)을 다시 돈다. Enter 는 상위→하위, 상태 안에서는 선언 순으로 돌기 때문에(`:3816`) 같은 상태에서 목표 태스크가 제목보다 먼저 선언돼 있거나, 목표가 대상보다 위 조상 상태에 걸려 있으면(조상은 Enter 가 다시 불리지 않는다) 그 목표가 지워지고 상태가 끝날 때까지 복구되지 않는다 — `WxStateTreeTask_SetQuestObjective.h:28` 의 "상태에 머무는 동안 유지" 계약 위반이다. 그 외 경우에도 핸들 재발급과 `OnJournalChanged` 다중 발화가 남는다. (b) `StartNextQuest` 가 재진입하면 다음 틱 예약이 한 번 더 쌓여 발견 6 의 이중 활성화로 이어진다. 엔진 doc-comment(`StateTreeTaskBase.h:107-112`)는 자식 상태 동안 유지되는 리소스형 태스크는 false 여야 한다고 명시하며, 같은 모듈의 `WxStateTreeTask_WaitMoveToTarget.cpp:16` 은 이미 false 로 둔다.
- **제안**: 세 태스크 생성자에 `bShouldStateChangeOnReselect = false;` 를 추가한다.
- **확신도**: 중간(엔진 동작은 소스로 확인했으나, 현재 퀘스트 에셋에 활성 상태를 대상으로 하는 전이가 있는지는 미확인이다)

### 3. 🟡 `WaitMoveToTarget` 은 빈 로케이터나 사라진 대상에도 영원히 Running — 퀘스트가 흔적 없이 멈춘다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23-28`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:48-54`
- **범주**: 버그/정확성
- **문제**: 빈 로케이터는 진입 시 경고 한 줄만 남기고 `Running` 을 반환해(`:23-28`) 완료될 수 없는 상태에 머문다. 대상이 해석되지 않는 경우도 `:48-54` 에서 로그 없이 Running 을 이어간다. WP 셀 미로드는 플레이어가 다가가면 풀리므로 Running 이 맞지만(`SyncFind` 는 강제 로드하지 않는다 — `WxStateTreeTask_WaitMoveToTarget.h:33`), 액터가 삭제·이름 변경돼 로케이터가 영구히 깨진 경우도 똑같이 무기한 대기한다. 이 태스크는 완료 판정에 참여하므로 `Failed` 반환이 상태에 실제로 전파되는데도 그 수단을 쓰지 않는다. 스텝 상태의 완료를 내는 태스크가 이것이라 결과는 "그 스텝에서 영구 정지"이고 원인 로그도 없다.
- **제안**: 빈 로케이터는 `EnterState` 에서 `Failed` 로 끝낸다. 해석 실패는 스트리밍을 고려해 Running 을 유지하되, 일정 시간 이상 지속되면 대상 경로를 담은 1회성 경고를 남긴다.
- **확신도**: 중간(빈 로케이터를 경고만 하고 넘기는 것은 `WxStateTreeTask_WaitMoveToTarget.h:30` 에 적힌 현재 의도라 방침 변경에 해당한다)

### 4. 🟡 `bHasActiveQuest` 가 "퀘스트 활성"이 아니라 "제목 등록됨"을 뜻한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:100`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:54`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:83-86`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:137-140`
- **범주**: 설계/구조
- **문제**: 이 플래그가 참이 되는 곳은 `SetQuestTitle`(`:54`)뿐이다. 제목 태스크 없이 목표만 거는 퀘스트나 제목을 뒤쪽 상태에서 거는 퀘스트는 러너가 돌고 목표가 저널에 올라와 있어도 `HasActiveQuest()` 가 false 이고, 이 값을 표시 조건으로 쓰는 구독자(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:53`)에선 "목표는 있는데 HUD 가 숨는" 형태로 드러난다. 저널 내용이나 러너 상태에서 조회할 수 있는 값을 별도 플래그로 저장해 동기화 책임만 늘린 형태이며, `ClearJournal` 의 조기 반환(`:137-140`)도 같은 플래그에 묶여 있다.
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
- **문제**: `SetTimerForNextTick` 은 호출마다 새 `FTimerData` 를 만들 뿐 중복 제거가 없고(`TimerManager.cpp:736-749`), 반환 핸들을 버리므로 앞선 예약을 취소·대체할 수 없다. 예약부터 실행까지의 한 프레임 창에 요청이 겹치면 전부 순서대로 실행된다. 발견 2 의 재진입으로 같은 요청이 두 번 쌓이면 다음 틱에 새 퀘스트가 시작 직후 한 번 더 재시작되고, 예약 직후 트리거 볼륨이 `StartQuest` 로 다른 퀘스트를 즉시 시작하면 뒤늦은 예약이 그 퀘스트를 덮는다. 어느 경우든 덮이는 쪽의 진입 상태 부수효과는 이미 실행된 뒤이며, 퀘스트 ST 에는 `FWxStateTreeTask_GiveRewards`(WxInventory)·`FWxStateTreeTask_TriggerSpawners`(WxWorld) 같은 되돌릴 수 없는 노드가 붙는다. 발견 1 의 제안대로 `StartQuest` 를 이 경로로 모으면 노출 빈도도 늘어난다.
- **제안**: 예약 `FTimerHandle` 을 멤버로 두고, 새 예약 전과 `ActivateQuest` 즉시 경로에서 기존 예약을 `ClearTimer` 해 마지막 요청만 유효하게 한다.
- **확신도**: 중간(누적·취소 불가는 소스로 확인했고, 실제 충돌은 한 프레임 창에 요청이 겹칠 때만 드러난다)

### 7. 🟢 "완료 판정에서 빠져 있어 상태를 끝내지 않는다" doc-comment 에 전제가 빠졌다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h:26`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h:30`
- **범주**: 버그/정확성
- **문제**: 태스크 자신의 기여만 보면 맞지만, 상태에 완료 판정 참여 태스크가 하나도 없으면 성립하지 않는다. UE 5.8 컴파일러는 그런 상태에 "상태 자신" 비트를 태스크 비트 뒤(오프셋 = 태스크 수)에 붙이는데(`StateTreeCompiler.cpp:390-400`), 상태 진입 시 `ResetStatus(TasksNum)` 은 태스크 수만큼만 비트를 지운다(`StateTreeTasksStatus.cpp:177-181`, `StateTreeTasksStatus.h:245-265`). 같은 부모의 형제 상태들은 시작 비트를 공유하므로(`StateTreeCompiler.cpp:1353-1357`), 직전 형제가 그 위치에 남긴 완료 비트 — 비참여 태스크의 `Succeeded` 반환도 비트 자체는 기록한다(`StateTreeTasksStatus.h:212-226`) — 를 물려받아 예컨대 목표 태스크만 얹은 스텝 상태가 진입 직후 완료로 판정된다(`StateTreeExecutionContext.cpp:4876-4881`). 새 퀘스트 상태를 짜는 사람이 이 문장을 근거로 대기 태스크 없는 상태를 만들면 그대로 함정에 빠진다.
- **제안**: 두 문장에 "단, 그 상태에 완료 판정 참여 태스크(대기 계열)가 최소 하나 있어야 한다"는 전제를 덧붙인다.
- **확신도**: 중간(엔진 메커니즘은 소스로 확인했으나, 현재 퀘스트 에셋에 판정 참여 태스크 없는 상태가 있는지는 미확인이다)

### 8. 🟢 인라인 예외 주석이 존재하지 않는 "코딩 규칙 4"를 근거로 든다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h:12`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h:12`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h:13`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h:13`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 는 `GetInstanceDataType()` 헤더 정의를 예외로 허용하되 그 지점에 예외 사유 주석을 요구한다. 네 헤더에 주석과 사유는 있지만 근거를 "코딩 규칙 4"로 적는데, 커밋 `5fe1ceb6` 에서 람다 규칙이 빠지며 인라인 금지는 규칙 3 이 됐고 규칙 4 는 더 이상 없어 주석이 가리키는 근거가 끊겼다. 같은 문구가 저장소의 다른 StateTree 노드 헤더 23곳(WxWorld·WxInventory·WxUI·WxDialogue)에도 있다.
- **제안**: 번호 대신 "인라인 함수 정의 금지 규칙의 예외"처럼 내용으로 지칭하도록 저장소 전체를 일괄 치환한다. 규칙 번호가 다시 바뀌어도 깨지지 않는다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`
- **훑은 파일**: 태스크 4종 헤더(`Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_*.h`), `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`. 참고용(리뷰 대상 아님)으로 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`(구독자 영향), 저장소 전체의 `UWxQuestComponent`·`ActivateQuest` 호출처(부착은 `Source/WxGame/Framework/WxGameState.cpp` 1곳뿐), `Content/` 에셋의 `StartQuest` 참조 문자열(`BP_QuestVolume` 1곳뿐), UE 5.8 엔진의 `StateTreeComponent.cpp`·`StateTreeExecutionContext.cpp`(Stop·EnterState·ExitState·틱 완료 집계)·`StateTreeTasksStatus.h/.cpp`·`StateTreeCompiler.cpp`·`StateTreeTaskBase.h`·`TimerManager.cpp`·`GameModeBase.cpp`·`WorldSettings.cpp`·`EngineUtils.h`·`LevelActor.cpp`·`Actor.cpp`·`PrimitiveComponent.cpp`(BeginPlay 순서·초기 오버랩)를 확인했다.
- **미검토 / 한계**: 퀘스트 `UStateTree` 에셋(`Content/Quest/ST_Quest_*`, `Content/Quest/Steps/ST_QuestStep_*`)과 `BP_QuestVolume` 내부는 에셋/BP 영역이라 열지 않았으므로, 발견 1·2·6·7 이 현재 저작물·맵 배치에서 실제로 성립하는지는 미정이다. 같은 에셋으로 `StartQuest` 를 다시 부르면 진행 중인 퀘스트가 처음부터 재시작(진입 부수효과 재실행)되는데 C++ 에는 이를 막는 장치가 없고 호출 측 BP 의 1회성 처리에 기대는 구조라 발견으로 잡지 않았다. 저널 리플리케이션 부재는 `WxQuestComponent.h:36` 에 명시된 v1(싱글/리슨 호스트) 유보라 발견에서 뺐다 — 데디케이티드 서버로 가면 `WaitMoveToTarget` 의 0번 컨트롤러 전제(`WxStateTreeTask_WaitMoveToTarget.cpp:41`)와 함께 재설계 대상이다. 퀘스트 진행도 영속화는 존재하지 않는 기능이라 발견에서 뺐다. 지난 리뷰(`9d8cb2dd`) 이후 모듈 소스 변경은 없으며, 이전 발견 7건은 모두 현재 코드·엔진 소스로 다시 검증해 유효했고 라인도 그대로다. 이번 판에서는 발견 1 에 "권위 측 BeginPlay 이전 호출" 경로를 엔진 근거와 함께 보강했고(이전 판은 조기 반환 목록에 한 단어로만 언급), 발견 8(낡은 규칙 번호)을 새로 추가했다.

---
*문서 기준 커밋 `5eb1a754` · 리뷰일 2026-09-17 · 소스 14파일 — `/module-review`로 갱신*
