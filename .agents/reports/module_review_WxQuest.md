# WxQuest — 코드 리뷰

> 14파일 · 실코드 500줄 남짓의 작은 모듈이고, 책임 분할(러너 소유 = 컴포넌트, 완료 판정 = Wait 계열 하나, 저널 정리 = 실행 상태 통지 한 곳)이 명확해 구조적 건강도는 좋다. 크래시성 결함·수명주기 위험·모듈 경계 침범은 없고, 남은 문제는 거의 전부 "StateTree 엔진이 실패를 조용히 삼키는 지점을 이 모듈이 확인하지 않는다"는 한 가지 축에 모여 있다. 이번 리뷰는 모듈 소스 14파일을 전부 읽고 컴포넌트·태스크 4종 cpp 를 깊게 봤으며, 판정 근거인 태스크 플래그·Enter/Exit/Stop/Start 실행 순서는 UE 5.8 설치본 엔진 소스에서 직접 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 저널 태스크 3종이 `bShouldStateChangeOnReselect` 기본값(true)을 그대로 둬 스텝 전이마다 저널이 통째로 재구축된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:10`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:10`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp:9`
- **범주**: 버그/정확성
- **문제**: 엔진은 전이 후에도 활성으로 남는 상태(Sustained)의 태스크에 대해 `bShouldStateChangeOnReselect` 가 true 면 `ExitState`·`EnterState` 를 다시 호출한다(UE 5.8 `StateTreeExecutionContext.cpp:3839`, `:4029`). 이 모듈에서 플래그를 끈 것은 `WaitMoveToTarget` 하나뿐이고(`WxStateTreeTask_WaitMoveToTarget.cpp:16`), 저널 태스크 3종은 전부 기본값이다. README 가 규정한 표준 조립(루트에 `SetQuestTitle`, 각 스텝 상태에 `SetQuestObjective`)에서 스텝이 넘어갈 때마다 루트가 Sustained 로 재진입해 `SetQuestTitle::EnterState` → `Objectives.Reset()`(`WxQuestComponent.cpp:53`)이 다시 돌고, 살아 있는 부모 상태의 목표들도 제거→재추가되어 핸들이 매번 새로 발급된다. 전이 1회에 `OnJournalChanged` 가 3~N+2회 발화한다.
  결과가 지금 맞아떨어지는 것은 엔진이 EnterState 를 루트→리프 순으로 돌아 "먼저 비우고 전부 다시 채우는" 형태가 되기 때문일 뿐이다. 같은 상태에서 `SetQuestObjective` 를 `SetQuestTitle` 보다 앞에 배치하면 그 목표는 조용히 사라지고, 둘 중 하나만 플래그를 끄면 목표가 중복되거나 유실된다 — 저작자가 알 수 없는 순서 의존이다.
  `StartNextQuest` 도 같다. 체인 종점 상태가 재선택되면 `EnterState` 가 다시 불려 `RequestActivateQuest` 가 한 번 더 예약된다(발견 4 와 연결).
- **제안**: 세 태스크 생성자에 `bShouldStateChangeOnReselect = false` 를 명시한다. `SetQuestObjective` 는 이 값이 "목표의 수명 = 상태의 수명"이라는 헤더 주석(`WxStateTreeTask_SetQuestObjective.h:29`)과 정확히 일치하는 의미이며, `SetQuestTitle` 의 파괴적인 `Objectives.Reset()` 도 실제로 퀘스트가 바뀔 때만 돌게 된다.
- **확신도**: 높음 (재진입 메커니즘은 엔진 소스로 확인)

### 2. 🟡 BP 에 열린 유일한 수주 경로가 콜스택-불안전한 `ActivateQuest` 로 직결된다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp:16`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:53`
- **범주**: 설계/구조
- **문제**: 모듈은 ST 실행 콜스택 안에서의 활성화를 위해 `RequestActivateQuest`(다음 틱 예약)를 따로 두고 `StartNextQuest` 태스크만 그것을 쓴다. 그런데 디자이너에게 노출된 단 하나의 진입점 `UWxQuestLibrary::StartQuest` 는 경고 문구도 없이 즉시 경로인 `ActivateQuest` 를 부른다. 퀘스트 ST 가 구동하는 BP(도메인 BP 태스크, 그로부터 호출되는 액터 이벤트 등)에서 이 노드를 쓰면 `StopLogic` 은 재진입 컨텍스트로 성공해 트리를 멈추고, `SetStateTreeReference` 도 상태가 Stopped 라 새 에셋으로 교체되지만, 이어지는 `StartLogic` 은 `"Reentrant call to StartTree is not allowed"` 에러 로그만 남기고 반환한다(UE 5.8 `StateTreeComponent.cpp:183`). 즉 **이전 퀘스트는 죽고 새 퀘스트는 시작되지 않은 채 `LogWxQuest` 에는 아무 흔적도 남지 않는다.**
- **제안**: `UWxQuestLibrary::StartQuest` 를 `RequestActivateQuest` 로 라우팅한다. 트리거 볼륨 수주가 한 틱 늦어지는 것은 체감되지 않고, 두 진입점의 안전성 차이를 없앨 수 있다.
- **확신도**: 중간 (경로는 확실하나 실제 저작에서 ST 안 호출이 얼마나 흔한지는 BP 범위 밖)

### 3. 🟡 `ActivateQuest` 의 3단계가 모두 실패를 무시하며, 실패 시 직전 퀘스트를 재시작할 수 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:34`
- **범주**: 버그/정확성
- **문제**: `StopLogic` → `SetStateTreeReference` → `StartLogic` 세 함수 모두 void 이고 실패 시 엔진 로그만 남긴다. 특히 `StopLogic` 은 내부 `SetContextRequirements` 가 실패하면 `StopTree` 를 아예 호출하지 않아 `bIsRunning` 이 true 로 남는다(UE 5.8 `StateTreeComponent.cpp:253`). 그러면 바로 다음 `SetStateTreeReference` 가 "running instance" 사유로 거부되고(`:495`) `StateTreeRef` 는 **구 퀘스트 에셋 그대로**인데, 이어진 `StartLogic()` 이 그 구 에셋을 다시 시작한다. 호출자 `UWxQuestLibrary::StartQuest` 는 void 라 이 상황을 전달할 수도 없다. 모듈에는 정확히 이런 조립 오류용 `LogWxQuest` 카테고리가 있고 태스크들은 쓰고 있는데, 정작 실행기 본체는 쓰지 않는다.
- **제안**: `StartLogic()` 직후 `QuestStateTree->IsRunning()`(또는 `GetStateTreeRunStatus()`)을 확인하고, 아니면 퀘스트 에셋 이름과 함께 `LogWxQuest` Warning 을 남긴다.
- **확신도**: 중간 (연쇄가 성립하려면 `SetContextRequirements` 실패 — 퀘스트 ST 스키마의 Context Actor Class 오설정 등 — 이 전제)

### 4. 🟡 `RequestActivateQuest` 는 예약을 병합·취소하지 않아 같은 퀘스트를 두 번 시작할 수 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:47`
- **범주**: 버그/정확성
- **문제**: 매 호출마다 `SetTimerForNextTick` 을 새로 잡고 핸들을 보관하지 않는다. 한 프레임에 요청이 두 번 들어오면 다음 틱에 `ActivateQuest` 가 두 번 실행되고, 두 번째가 **방금 시작한 새 퀘스트**를 `StopLogic`→`StartLogic` 한다. 새 퀘스트 첫 상태의 부수효과(다른 도메인 ST 태스크가 거는 보상 지급·스폰 등)가 두 번 실행되고, 저널도 한 프레임 안에서 구축→파기→재구축된다. 발견 1 의 재진입(체인 종점 상태 재선택 시 `StartNextQuest::EnterState` 재호출)이 이 상황을 만드는 가장 현실적인 경로다.
- **제안**: 보류 중인 요청을 `TSoftObjectPtr` 필드 + `FTimerHandle` 하나로 들고, 이미 예약돼 있으면 대상만 덮어쓴다.
- **확신도**: 중간

### 5. 🟡 빈 로케이터·미해석 대상이면 `WaitMoveToTarget` 이 영원히 Running — 퀘스트가 복구 경로 없이 멈춘다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:23`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:48`
- **범주**: 버그/정확성
- **문제**: 로케이터가 비어 있으면 `EnterState` 가 Warning 한 줄만 남기고 `Running` 을 돌려준다. 이 모듈에서 상태를 끝내는 유일한 태스크이므로, 잘못 조립된 퀘스트는 완료 조건이 영영 성립하지 않는 채로 그 상태에 고정된다 — 플레이어에겐 목표가 떠 있는데 무엇을 해도 진행되지 않는 소프트락이고, 다음 퀘스트 체인도 함께 끊긴다. `Instance.Target.SyncFind(Owner)` 가 계속 null 을 돌려주는 경우(삭제된 액터를 가리키는 낡은 로케이터)도 로그 한 줄 없이 같은 결과가 된다.
- **제안**: 빈 로케이터는 `EnterState` 에서 `Failed` 를 반환해 상태가 실패 전이를 타게 하고(이 태스크는 완료 판정에 포함되므로 실제로 전달된다 — 발견 6 과 대비), 해석 실패가 일정 시간 이상 지속되면 `LogWxQuest` Warning 을 한 번 남긴다.
- **확신도**: 중간 (WP 스트리밍 타이밍상 일시적 미해석을 견디려고 Running 을 유지했을 수 있음)

### 6. 🟡 저널 태스크가 돌려주는 `Failed` 는 엔진이 삼켜서, 헤더가 약속한 동작이 일어나지 않는다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h:27`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp:28`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp:28`
- **범주**: 버그/정확성
- **문제**: 세 태스크 모두 `bConsideredForCompletion = false` 다. 엔진은 완료 판정에서 빠진 태스크의 `EnterState` 반환값을 상태 결과에 반영하지 않는다(UE 5.8 `StateTreeExecutionContext.cpp:3873` 의 `IsConsideredForCompletion` 가드). 따라서 `StartNextQuest` 헤더의 *"퀘스트 컴포넌트가 없으면 잘못된 조립(퀘스트 러너 밖 사용)이라 예약 없이 Failed 로 끝난다"* 는 서술은 실제와 다르다. 퀘스트 컴포넌트를 못 찾아도 상태는 아무 일 없이 계속되고, `StartNextQuest` 는 나머지 둘과 달리 로그조차 남기지 않아(`WxStateTreeTask_StartNextQuest.cpp:25`) 체인이 끊긴 사실이 어디에도 드러나지 않는다.
- **제안**: (a) 세 태스크의 `Failed` 반환은 유지하되 주석에서 "상태를 끝낸다"는 뉘앙스를 걷어내고, (b) `StartNextQuest` 에도 나머지 둘과 같은 `LogWxQuest` Warning 을 추가한다. 조립 오류를 실제로 퀘스트 실패로 만들고 싶다면 이 태스크만 `bConsideredForCompletion` 을 켜야 한다.
- **확신도**: 높음

### 7. 🟢 `bHasActiveQuest` 는 "퀘스트 활성"이 아니라 "제목이 등록됨"을 뜻한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:54`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:137`
- **범주**: 설계/구조
- **문제**: 플래그를 켜는 곳이 `SetQuestTitle` 하나뿐이다. `SetQuestTitle` 태스크를 빠뜨린 퀘스트는 러너가 멀쩡히 돌고 목표도 등록되는데 `HasActiveQuest()` 가 false 라, 구독자인 `UWxViewModel_Quest` 가 저널 위젯을 열지 않는다(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:53`). 게다가 `ClearJournal` 이 같은 플래그로 조기 반환하므로 그 퀘스트가 끝나도 정리 통지가 나가지 않는다. 실제 권위 있는 활성 여부는 러너가 쥐고 있다.
- **제안**: `HasActiveQuest()` 가 `QuestStateTree && QuestStateTree->IsRunning()` 을 반환하게 하고 별도 저장 필드를 없앤다. 조회 가능한 값을 따로 들지 않는다는 프로젝트 선호와도 맞는다.
- **확신도**: 중간

### 8. 🟢 저널이 복제되지 않아 권위가 아닌 머신에는 퀘스트 UI 가 비어 있다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h:93`
- **범주**: 설계/구조
- **문제**: `QuestTitle`·`Objectives`·`bHasActiveQuest` 어느 것도 복제되지 않고, 러너 자체가 권위에서만 생성된다. 싱글·리슨 호스트에서는 문제가 없지만 클라이언트는 저널이 영원히 빈 채로 남는다. README 가 v1 전제로 명시한 알려진 한계이므로 결함으로 보지는 않되, 프로젝트 목표(최대 4인 멀티)상 언젠가 반드시 걸린다. `FText` 배열이라 그대로 복제하기 어려워 스냅샷 구조체 설계가 선행돼야 하는 만큼 미룰수록 비용이 커진다.
- **제안**: 지금 손댈 필요는 없다. 다만 복제 스냅샷(제목 + 목표 텍스트 배열)을 `FWxQuestObjective` 확장으로 미리 설계해 두면, 태스크·저널 API 는 그대로 두고 복제 경로만 덧붙일 수 있다.
- **확신도**: 낮음 (의도된 설계일 수 있음 — README 에 v1 전제로 명시돼 있다)

### 9. 🟢 인라인 예외 주석이 존재하지 않는 규칙 번호를 인용한다
- **위치**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h:12`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h:12`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h:13`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h:13`
- **범주**: 규칙 위반
- **문제**: 네 헤더 모두 `GetInstanceDataType()` 의 헤더 정의를 *"코딩 규칙 4 의 예외"* 로 적고 있으나, CLAUDE.md 코딩 규칙은 3개뿐이고 인라인 금지는 3번이다. 예외 사유를 남기라는 요구 자체는 충족하지만 근거 번호가 어긋나 있어, 규칙이 한 번 더 바뀌면 추적이 끊긴다.
- **제안**: 네 곳의 "코딩 규칙 4"를 "코딩 규칙 3"으로 고치거나, 번호 없이 "인라인 함수 정의 금지 규칙의 예외"로 바꾼다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, 태스크 4종 헤더, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`, `Plugins/WxQuest/README.md`
- **대조 확인(발견 없음)**: 모듈 경계는 깨끗하다 — `.Build.cs` 의존성은 엔진 모듈 + `WxCore` 뿐이고, 실제 include 도 `WxLocatorUtils.h`(WxCore) 하나만 모듈 밖을 향한다. 저작권 첫 줄은 14파일 전부 충족하고, `Wx` 접두사·`Handle` 콜백 접두사·`BlueprintCallable` 사용처(BP Function Library 1건)도 규칙을 지킨다. 런타임 생성한 `QuestStateTree` 가 `InitializeComponent`/`BeginPlay` 를 제대로 받는지, `bConsideredForCompletion` 이 쿠킹 후에도 유지되는지(에디터 전용 UPROPERTY 로 ST 컴파일 시 베이킹됨)도 엔진 소스로 확인했고 둘 다 문제없다.
- **미검토 / 한계**: 퀘스트 `UStateTree` 에셋의 실제 상태·태스크 배치는 BP/에셋 범위라 보지 않았다 — 발견 1·5·6 의 실제 체감 정도는 에셋이 어떤 모양인지에 달려 있다. `FUniversalObjectLocator::SyncFind` 의 매 틱 비용은 프로파일링으로 재지 않고 "경로 조회라 비용이 무시된다"는 README 전제를 받아들였다(활성 퀘스트가 1개뿐이라 실측 필요성은 낮다). `OnJournalChanged` 가 목표 1개 갱신마다 발화해 `UWxViewModel_Quest::RebuildObjectives` 가 매번 목표 VM 을 전부 재생성하는 비용은 WxGame 리뷰 소관으로 넘긴다.

---
*문서 기준 커밋 `fe57e17a` · 리뷰일 2026-09-20 · 소스 14파일 — `/module-review`로 갱신*
