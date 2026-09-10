# WxQuest — 코드 리뷰

> 14파일짜리 작은 모듈이고 권위 모델·에셋 불가지·"목표 수명 = 상태 수명" 규약이 클래스 doc-comment 에 정리돼 있어 구조는 건강하다. 크래시성 결함이나 수명주기 위험(댕글링·GC)은 없고, 남은 위험은 전부 "조용히 어긋나는" 쪽 — 코드가 StateTree 의 재진입·완료 판정 규약을 전제만 하고 확인하지 않는 지점들이다. 이번 리뷰는 모듈 소스 14파일(헤더 7 + cpp 7)을 전부 읽고 `WxQuestComponent.cpp` 와 태스크 4종 cpp 를 깊게 봤으며, 판정 근거가 되는 엔진 동작은 UE 5.8 설치본의 `StateTreeExecutionContext.cpp`·`StateTreeComponent.cpp`·`StateTreeTaskBase.h`·`TimerManager.cpp` 에서 직접 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 저널 태스크가 `bShouldStateChangeOnReselect` 를 켠 채로 둬 Sustained 재진입에 무방비다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:10-18`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:10-18`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:53`
- **범주**: 버그/정확성
- **문제**: 두 Set 태스크의 생성자는 `bShouldCallTick` 과 완료 판정 플래그만 손대고 `bShouldStateChangeOnReselect` 를 엔진 기본값 `true` 로 남긴다. UE 5.8 `StateTreeExecutionContext.cpp:3839-3840`(EnterState)·`:4028-4029`(ExitState)는 `ChangeType == Sustained`(부모는 활성 유지, 자식만 교체되는 전이)일 때 이 플래그가 참인 태스크에만 진입·이탈을 다시 부른다. 따라서 "제목·목표는 부모 상태, 스텝은 자식 상태" 조립에서 자식이 바뀔 때마다 목표 태스크가 `RemoveObjective` → `AddObjective` 를 돌아 핸들이 새로 발급되고, 제목 태스크도 재진입해 `SetQuestTitle` 안의 `Objectives.Reset()`(`WxQuestComponent.cpp:53`)을 다시 돈다. 엔진의 전이 순서(Exit 전량 → Enter 전량, Enter 는 부모 상태부터)를 따라가 보면 피해 정도가 조립에 따라 갈린다. (a) 목표가 교체되는 자식 쪽에만 있으면 이미 Exit 된 뒤라 데이터 손상은 없고 전이마다 핸들 재발급 + `OnJournalChanged` 다중 발화만 남는다 — 구독자는 브로드캐스트마다 목표 뷰모델 UObject 를 통째로 재할당한다(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:51-62`). (b) 그러나 제목 태스크보다 오래 사는 상태(조상·형제)가 목표를 걸고 있으면, 제목 재진입의 `Objectives.Reset()` 이 **아직 살아 있는 목표를 저널에서 지워 버리고** 그 목표는 자기 상태가 끝날 때까지 복구되지 않는다 — `WxStateTreeTask_SetQuestObjective.h:28` 이 선언한 "상태에 머무는 동안 유지" 계약과 정면으로 어긋난다. 엔진 `StateTreeTaskBase.h:107-113` 의 doc-comment 자체가 "자식 상태에서 획득될 것으로 기대되는 리소스 점유형 태스크는 false 여야 한다"고 못 박고 있고, 같은 모듈의 `WxStateTreeTask_WaitMoveToTarget.cpp:16` 은 이 플래그를 명시적으로 껐다 — 저널 태스크 3종만 빠졌다.
- **제안**: `SetQuestTitle`·`SetQuestObjective`(그리고 발견 5 를 함께 막으려면 `StartNextQuest` 까지) 생성자에 `bShouldStateChangeOnReselect = false;` 를 추가한다. 제목 태스크의 `Objectives.Reset()` 도 발견 6 과 함께 재검토한다(정상 경로에선 `ClearJournal` 이 이미 비운 뒤라 중복이다).
- **확신도**: 높음(엔진 동작은 소스로 확인). 단 (b) 시나리오가 현재 퀘스트 에셋에서 실제로 성립하는지는 미확인 — 에셋이 스텝별 링크 트리 구성이면 (a) 수준의 churn 에 그친다.

### 2. 🟡 완료 판정에서 뺀 태스크의 `Failed` 는 엔진이 무시한다 — `StartNextQuest` 는 로그도 없이 체인이 끊긴다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:21-26`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:24-29`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:24-29`
- **범주**: 버그/정확성
- **문제**: 세 태스크는 생성자에서 `bConsideredForCompletion = false` 로 완료 판정에서 스스로 빠져 놓고, 퀘스트 컴포넌트를 못 찾으면 `EStateTreeRunStatus::Failed` 를 돌려준다. 그러나 엔진 `StateTreeExecutionContext.cpp:3873-3879` 의 EnterState 태스크 루프는 `if (CurrentStateTasksStatus.IsConsideredForCompletion(StateTaskIndex))` 안에서만 반환값을 상태 결과에 반영한다 — 완료 판정에서 빠진 태스크의 `Failed` 는 상태 진입을 막지 못하고 트리는 잘못 조립된 채 계속 굴러간다(`bConsideredForCompletion` 은 `StateTreeTaskBase.h:146-157` 기준 에디터 전용 데이터이며 컴파일된 에셋의 완료 마스크로 굳는다). 제목·목표 태스크는 그래도 `LogWxQuest` 경고를 남기지만 `StartNextQuest` 는 조기 반환에 로그가 하나도 없어(`WxStateTreeTask_StartNextQuest.cpp:23-26`) 퀘스트 체인이 아무 흔적 없이 끊긴다. 헤더 문서(`WxStateTreeTask_StartNextQuest.h:27`)가 약속한 "예약 없이 Failed 로 끝난다"의 뒷부분은 실제로 일어나지 않는다.
- **제안**: `StartNextQuest` 의 조기 반환에 형제 태스크와 같은 `LogWxQuest` 경고를 넣고, `WxStateTreeTask_StartNextQuest.h:27` 의 서술을 실제 동작(경고만 남기고 상태는 계속 진행)대로 정정한다. 실패를 상태에 실제로 전파하려면 `bConsideredForCompletion = false` 전제를 버려야 하는데, 그러면 Set 계열이 진입 즉시 상태를 끝내 버려 현재 설계와 양립하지 않는다.
- **확신도**: 높음

### 3. 🟡 `WaitMoveToTarget` 은 대상 해석에 실패해도 영원히 Running — 퀘스트가 조용히 멈춘다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23-28`, `:48-54`
- **범주**: 버그/정확성
- **문제**: 빈 로케이터는 진입 시 경고 한 줄만 남기고 `Running` 을 반환해(`:23-28`) 완료될 수 없는 상태에 그대로 머문다. 대상이 해석되지 않는 경우(`SyncFind` 가 null — 액터 삭제·WP 셀 미로드·PIE 경로 불일치)도 `:48-54` 에서 아무 로그 없이 Running 을 이어간다. 헤더(`WxStateTreeTask_WaitMoveToTarget.h:34`)가 밝혔듯 `SyncFind` 는 강제 로드를 하지 않으므로 미로드 셀의 액터는 정상적으로 null 이 나온다. 스텝 상태의 완료를 내는 유일한 태스크가 이것이므로 결과는 "퀘스트가 그 스텝에서 영구 정지"이고, 로그가 없어 원인 추적도 어렵다. 발견 2 와 달리 이 태스크는 완료 판정 대상이라 `Failed` 반환이 실제로 상태에 전파된다 — 즉 고칠 수단이 있다.
- **제안**: 빈 로케이터는 `EnterState` 에서 `Failed` 로 끝낸다. 해석 실패는 최소 1회성 경고를 남기거나, 허용 시간을 넘기면 실패로 떨어뜨린다.
- **확신도**: 중간(빈 로케이터를 경고만 하고 넘기는 것은 `WxStateTreeTask_WaitMoveToTarget.h:31` 에 적힌 현재 의도이므로 방침 변경에 해당)

### 4. 🟡 `ActivateQuest` 가 세 단계 중 어느 것의 성공도 확인하지 않고, 그 경로를 BP 에 무방비로 노출한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:18-37`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h:23-24`
- **범주**: 설계/구조
- **문제**: `StopLogic` → `SetStateTreeReference` → `StartLogic`(`:34-36`) 세 호출 중 어느 것도 결과를 보지 않는다. 엔진 `StateTreeComponent.cpp` 에서 실패 연쇄를 확인했다 — 러너 실행 콜스택 안에서 불리면 `StopLogic`(`:246-250`)은 정지를 프레임 끝으로 미루고, `SetStateTreeReference`(`:491-502`)는 인스턴스 실행 상태가 아직 `Running` 이면 "Trying to change the state tree on a running instance." 경고만 남기고 교체를 거부하며, 이어지는 `StartLogic`(`StartTree`, `:181-185`)도 "Reentrant call ... is not allowed." 로 반려된다. 결과는 크래시가 아니라 "새 퀘스트가 조용히 사라지고 저널만 비는" 상태다. 같은 파일 `:232-235` 의 `if (!bIsRunning) return;` 조기 반환 때문에, 인스턴스 상태는 Running 인데 `bIsRunning` 만 꺼진 경우(엔진이 `TickComponent:123-129` 에서 잘못된 에셋을 만나 `bIsRunning=false` 로 내려놓는 경로)에는 정지 없이 교체가 거부되고 **직전 퀘스트가 그대로 재시작**된다. 헤더 주석(`WxQuestComponent.h:53`)은 "ST 실행 콜스택 밖에서만 호출"을 못 박았지만, 실제 저작 진입점인 `UWxQuestLibrary::StartQuest` 는 이 무방비 경로를 BlueprintCallable 로 그대로 노출한다 — 안전한 짝인 `RequestActivateQuest`(`WxQuestComponent.h:56-57`)는 UFUNCTION 이 아니라 BP 에서 보이지 않으므로, 저작자에게는 규약을 어길 수단만 있고 지킬 수단이 없다. 같은 함수 앞부분의 조기 반환(`:20-29`)도 "널 에셋", "비-권위", "BeginPlay 전이라 러너가 아직 없음" 세 가지를 구분 없이 침묵으로 삼킨다.
- **제안**: `StopLogic` 뒤에 `QuestStateTree->GetStateTreeRunStatus() == EStateTreeRunStatus::Running`(public getter) 을 확인해 교체가 거부될 상황이면 `LogWxQuest` 에러를 남기고 중단한다. 더 나아가려면 러너가 실행 중일 때는 `RequestActivateQuest` 의 다음 틱 경로로 흘려보내 진입점 하나로 수렴시킨다.
- **확신도**: 중간(현재 C++ 에서 확인되는 호출자는 트리거 볼륨 계열과 `StartNextQuest` 의 안전 경로뿐이라 재진입은 BP 저작으로만 열린다)

### 5. 🟡 `RequestActivateQuest` 의 다음 틱 예약이 누적되고 취소할 수 없다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:39-48`
- **범주**: 버그/정확성
- **문제**: `SetTimerForNextTick` 을 호출할 때마다 새 타이머가 하나씩 쌓이고(엔진 `TimerModule` 의 `InternalSetTimerForNextTick` 은 매 호출 `FTimerData` 를 새로 만든다 — 델리게이트 중복 제거가 없다), 반환된 `FTimerHandle` 을 버리므로 이미 걸린 예약을 취소·대체할 수단도 없다. "예약"이라는 이름과 달리 실제 의미는 "무제한 큐"다. 세 가지 방식으로 어긋난다. (a) `StartNextQuest` 도 `bShouldStateChangeOnReselect` 가 기본값 `true` 라(발견 1 과 같은 뿌리) 그 태스크가 놓인 상태가 Sustained 재선택되면 예약이 한 번 더 들어가고, 같은 틱에 `ActivateQuest` 가 두 번 돌아 방금 시작한 퀘스트를 멈추고 다시 시작한다. (b) 예약 사이에 트리거 볼륨이 `UWxQuestLibrary::StartQuest` 로 다른 퀘스트를 시작하면, 뒤늦게 터진 예약이 플레이어가 방금 수주한 퀘스트를 조용히 덮어쓴다. (c) 두 경우 모두 덮이는 퀘스트의 진입 상태 부수효과는 이미 실행된 뒤다 — 퀘스트 ST 에는 `FWxStateTreeTask_GiveRewards`(WxInventory)·`FWxStateTreeTask_TriggerSpawners`(WxWorld) 처럼 되돌릴 수 없는 노드가 붙으므로 보상 지급·스폰이 헛돌 수 있다.
- **제안**: 예약 `FTimerHandle` 을 멤버로 들고 `SetTimerForNextTick` 전에 기존 예약을 `ClearTimer` 해 "마지막 요청만 유효"로 만든다. 즉시 경로(`ActivateQuest`)에서도 대기 중인 예약을 함께 지워, 나중 것이 앞선 결정을 되돌리지 않게 한다.
- **확신도**: 중간(타이머 누적·취소 불가는 소스로 확인했고, 실제 충돌은 같은 프레임·다음 프레임에 요청이 겹칠 때만 드러난다)

### 6. 🟡 `bHasActiveQuest` 가 "퀘스트 활성"과 "저널에 제목이 있음"을 겸업한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:100`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:54`, `:135-140`
- **범주**: 설계/구조
- **문제**: 이 플래그는 오직 `SetQuestTitle` 에서만 참이 된다(`WxQuestComponent.cpp:54`). 그래서 제목 태스크를 쓰지 않거나 제목을 뒤쪽 상태에서 거는 퀘스트는, 러너가 멀쩡히 돌고 목표까지 저널에 올라와 있는데도 `HasActiveQuest()` 가 false 다. 이 값을 HUD 표시 조건으로 쓰는 구독자(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:46`)에선 "목표는 있는데 저널이 안 뜨는" 형태로 드러난다. 게다가 `ClearJournal` 이 이 플래그로 조기 반환하므로(`:137-140`) 제목 없이 목표만 있던 저널은 종료 시 정리 통지조차 나가지 않는다. 진짜 "활성" 여부는 러너(`UStateTreeComponent::IsRunning()`)가 쥐고 있고 저널 유무는 `QuestTitle`/`Objectives` 에서 그대로 파생되므로, 이 플래그는 세 번째 진실 원본이다.
- **제안**: 플래그를 없애고 `HasActiveQuest()` 를 러너 상태에서, 저널 유무는 `QuestTitle`/`Objectives` 에서 파생시킨다. 최소한 `ClearJournal` 의 조기 반환 조건만이라도 "제목 또는 목표가 하나라도 있으면"으로 넓힌다.
- **확신도**: 중간(모든 퀘스트가 제목 태스크로 시작한다는 저작 관례가 전제라면 의도된 설계일 수 있음)

### 7. 🟢 다음 틱 예약 경로의 무검사 `GetWorld()` 와 게임 스레드 동기 로드
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:47`, `:132`
- **범주**: 성능/안전
- **문제**: `GetWorld()->GetTimerManager()` 가 반환값을 검사하지 않고 역참조한다(플레이 중엔 유효하지만 계약상 무방비). 이어지는 `HandleDeferredActivateQuest` 는 `LoadSynchronous()` 로 퀘스트 에셋과 그 하드 참조를 게임 스레드에서 동기 로드하므로 체인 전환 프레임에 히치가 생길 여지가 있다.
- **제안**: `GetWorld()` 널 검사를 추가하고, 히치가 관측되면 `FStreamableManager` 비동기 요청으로 바꾼다.
- **확신도**: 중간(에셋이 작으면 체감되지 않을 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`
- **훑은 파일**: 태스크 4종 헤더, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`. 참고용(리뷰 대상 아님)으로 `Source/WxGame/MVVM/WxViewModel_Quest.cpp`·`.h`(구독자 영향), `Source/WxGame/Framework/WxGameState.cpp`(부착 경로), UE 5.8 엔진의 `StateTreeExecutionContext.cpp`·`StateTreeTaskBase.h`·`StateTreeComponent.cpp`·`TimerManager.cpp` 를 확인했다.
- **규칙 점검 결과**: `CLAUDE.md` 코딩·모듈 규칙 위반은 발견되지 않았다. 저작권 첫 줄 15/15(Build.cs 포함) 준수, `Wx` prefix 준수, 델리게이트·타이머 콜백 `Handle` prefix 준수(`HandleStateTreeRunStatusChanged`, `HandleDeferredActivateQuest`), `BlueprintCallable` 은 `UWxQuestLibrary::StartQuest` 1건뿐이며 BP Function Library 소속, 헤더 인라인 정의는 `GetInstanceDataType()` 4건이고 전부 예외 사유 주석이 붙어 있으며, 람다·`FORCEINLINE` 사용 0건, 의존 모듈은 `WxCore` 와 엔진 모듈(`StateTreeModule`·`GameplayStateTreeModule`·`UniversalObjectLocator`)뿐이다. `UWxQuestComponent::BeginPlay` 는 `Super::` 를 부르고, ST 태스크의 `EnterState`/`ExitState` 는 엔진 기본 구현이 빈 함수라 `Super::` 미호출이 정상이다.
- **미검토 / 한계**: 퀘스트 `UStateTree` 에셋(`Content/Quest/ST_Quest_Main1`·`Main2`, `Content/Quest/Steps/ST_QuestStep_*`)의 상태 계층은 BP/에셋 영역이라 열지 않았다. 따라서 발견 1 의 (b) 시나리오와 발견 5 의 (a) 시나리오가 현재 저작물에서 실제로 성립하는지는 미정이다. 저널의 리플리케이션 부재는 `WxQuestComponent.h:36,43` 에 명시된 v1(싱글/리슨 호스트) 유보 사항이라 발견으로 잡지 않았다 — 데디케이티드 서버로 가면 클라 GameState 사본의 저널이 영구히 비고 `WaitMoveToTarget` 의 0번 컨트롤러 전제(`WxStateTreeTask_WaitMoveToTarget.cpp:41`)도 함께 재설계 대상이 된다. 퀘스트 진행도 영속화는 이 모듈에 존재하지 않는 기능이라 발견에서 뺐으나, 도입 시 저널이 러너의 라이브 상태에서만 파생된다는 점(복원할 "진행도"의 단일 지점이 없음)이 첫 설계 과제가 된다. 지난 리뷰(`262e4cca`) 이후 이 플러그인의 변경은 README 갱신뿐이라 소스 발견은 전부 현재 코드에서 다시 검증했다.

---
*문서 기준 커밋 `1d91a915` · 리뷰일 2026-09-10 · 소스 14파일 — `/module-review`로 갱신*
