# WxQuest — 퀘스트 시스템

## 한줄 요약

WxQuest는 StateTree로 퀘스트를 실행하고 플레이어에게 보여줄 제목과 목표를 관리합니다.


> 퀘스트 1개를 StateTree 에셋 1개로 보고, 그 진행을 서버 권위로 구동하며 플레이어에게 보여줄 저널(제목·목표 목록)을 유지한다. 퀘스트의 내용은 전부 에셋이 정하고, 이 모듈은 실행기와 저널만 제공한다.

## 책임
**담당**
- 활성 퀘스트 StateTree 의 실행·교체·정지 (동시 활성 퀘스트는 1개, 새 시작은 교체)
- 저널 상태(제목 1개 + 목표 N개) 보관과 변경 통지
- 퀘스트 저작용 StateTree 태스크 노드 제공 (제목/목표 등록, 도달 대기, 다음 퀘스트 체인)
- 레벨 배치 액터가 퀘스트를 수주시킬 수 있는 블루프린트 진입점

**경계 (비담당)**
- 저널을 화면에 그리는 일 — 뷰모델·위젯은 `WxGame`(`UWxViewModel_Quest`)과 [WxUI](../ui/index.md)가 담당하며, 이 모듈은 `OnJournalChanged` 통지만 쏜다
- 퀘스트 컴포넌트를 GameState 에 붙이는 일 — `WxGame` 의 `AWxGameState` 생성자가 기본 서브오브젝트로 생성한다
- 보상 지급·전투·대화 등 퀘스트가 걸어 놓는 실제 게임 동작 — [WxCombat](../combat/index.md), [WxInventory](../inventory/index.md), [WxDialogue](../dialogue/index.md) 등 각 도메인의 StateTree 노드로 조립한다
- 퀘스트 진행 상황의 저장/복원, 퀘스트 목록·수주 조건 같은 메타데이터 관리 (현재 없음)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | 이 모듈의 허브. 러너(`UStateTreeComponent`)를 권위 측에서만 런타임 생성해 소유하고, 모든 태스크가 오너에서 찾아 들어오는 저널 API 를 노출한다 | [Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h](../../../../Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h) |
| `UWxQuestLibrary` | 컴포넌트를 모르는 외부(레벨 배치 트리거 등)가 월드 GameState 를 거쳐 수주를 거는 유일한 BP 경로 | [Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h](../../../../Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h) |
| `FWxStateTreeTask_SetQuestTitle` | 퀘스트 루트 상태에 거는 저널 등록 태스크 | [Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h](../../../../Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h) |
| `FWxStateTreeTask_SetQuestObjective` | 목표의 수명을 상태의 수명에 묶는 태스크(진입 시 등록, 이탈 시 핸들로 제거) | [Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h](../../../../Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h) |
| `FWxStateTreeTask_WaitMoveToTarget` | 이 모듈이 가진 유일한 완료 판정 태스크. 상태를 끝내는 쪽은 항상 이런 Wait 계열이다 | [Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h](../../../../Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h) |
| `FWxStateTreeTask_StartNextQuest` | 퀘스트 체인 연결점. 러너 콜스택 안이라 즉시 교체가 아닌 다음 틱 예약으로 처리한다 | [Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h](../../../../Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h) |
| `LogWxQuest` | 조립 오류(러너 밖 태스크 사용, 빈 로케이터)를 알리는 모듈 로그 카테고리 | [Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h](../../../../Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h) |

## 확장 포인트 / 규약
- **새 퀘스트 만들기**: C++ 작업 없이 `UStateTree` 에셋 하나를 만들고, 루트에 `SetQuestTitle`, 각 스텝 상태에 `SetQuestObjective` + 완료를 내는 Wait 태스크를 얹는다. 마지막 상태에서 `StartNextQuest` 로 다음 에셋을 가리키면 체인이 되고, 비워 두면 종점이다.
- **새 태스크 만들기**: `FStateTreeTaskCommonBase` 를 상속하고, `Context.GetOwner()`(GameState)에서 `UWxQuestComponent` 를 `FindComponentByClass` 로 찾는 것이 이 모듈의 태스크 규약이다. 저널만 건드리는 태스크는 생성자에서 `bConsideredForCompletion = false` 로 완료 판정에서 빠져야 한다 — 그러지 않으면 `Succeeded` 반환이 상태를 즉시 끝낸다. 상태를 끝내는 책임은 Wait 계열 태스크 하나로 몰아 둔다.
- **레벨 액터 참조**: 태스크가 배치 액터를 가리킬 때는 `FUniversalObjectLocator`(WxCore 의 `WxLocatorUtils` 와 함께)를 쓴다. ST 컴파일러의 레벨 액터 참조 검증을 피하면서 WP 언로드/재로드를 견디기 위한 선택이다.
- **권위 모델(최대 4인 멀티)**: 러너는 권위 머신의 `BeginPlay` 에서만 생성되므로 비-권위에서는 `ActivateQuest`·태스크 진입이 자연히 노옵이다. 저널 자체는 리플리케이트되지 않으며 권위(싱글/리슨 호스트) 기준으로만 채워진다. `WaitMoveToTarget` 이 0번 플레이어 컨트롤러를 보는 것도 같은 v1 전제다.
- **수명 규약**: 저널 정리는 태스크가 아니라 러너의 `OnStateTreeRunStatusChanged` 한 곳에서 한다 — 완료·실패·교체 세 종료 경로가 모두 여기로 수렴한다. 목표는 문구가 아니라 발급 핸들로 지목하므로 중복 문구도 안전하다.

## 여기서부터 읽어라
1. [Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h](../../../../Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h) — 실행 모델(러너 소유, 권위 한정, 종료 수렴)이 클래스 주석에 다 들어 있다. 나머지 파일은 전부 이 컴포넌트를 향한다.
2. [Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp](../../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp) — 교체 시 `StopLogic`→`SetStateTreeReference`→`StartLogic` 순서와, 재진입 회피용 다음 틱 예약을 확인한다.
3. [Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp](../../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp) — 태스크가 오너에서 컴포넌트를 찾는 공통 패턴과 EnterState/ExitState 쌍의 수명 처리 표본.
4. [Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h](../../../../Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h) — 외부에서 퀘스트가 시작되는 유일한 경로.

## 관련 모듈
- 상위: `WxGame` — `AWxGameState` 가 컴포넌트를 소유하고 `UWxViewModel_Quest` 가 `OnJournalChanged` 를 구독한다. 표시 계층은 [WxUI](../ui/index.md), 퀘스트가 조립해 쓰는 동작 노드는 [WxCombat](../combat/index.md)·[WxInventory](../inventory/index.md)·[WxDialogue](../dialogue/index.md) 등 각 도메인에 있다.
- 기반: [WxCore](../foundation/index.md) — 로케이터 유틸 등 공용 정의.

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

> 상태: current · 범위: 본문 핵심 계약·주요 C++ 경로의 정적 재검증 · 2026-09-20 · 기준 커밋: 5a3f282e3bd71fd85d263021c113cd30558820b7
> 아래 검증 범위와 근거에 명시한 경로를 현재 작업 트리에서 확인했습니다. 전체 소스의 결함 검토·빌드·게임 실행·BP/WBP·DataTable·BT/StateTree 에셋 내부는 미검증입니다.
> [Wiki 목차](../../index.md) · [운영 절차](../../wiki-maintenance.md)

## 검증 범위와 근거

[Build.cs](../../../../Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs)·[descriptor](../../../../Plugins/WxQuest/WxQuest.uplugin), [QuestComponent](../../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp)의 권위 러너 생성·교체·저널 정리, [목표 등록/제거](../../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp), [다음 퀘스트 예약](../../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp), [도달 대기](../../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp)를 확인했다. 부착은 [WxGameState.cpp](../../../../Source/WxGame/Framework/WxGameState.cpp)에서 대조했다.

저널은 네트워크 복제 속성이 아니며, `WaitMoveToTarget`은 0번 플레이어를 조회한다. 위치 대상이 해석되지 않으면 Running에 남는다. `StartNextQuest`의 빈 참조는 새 퀘스트 요청을 생략하는 의미이며, 현재 트리의 종료 전이까지 자동으로 만드는 것은 아니다. 실제 퀘스트 에셋·로드/언로드·멀티 플레이 실행은 미검증이다.

*이관 원문의 기준 커밋 `047197a` · 생성일 2026-09-16 · 소스 14파일 — 원문 출처 보존; 현재 확인 범위는 이 참고 정보 참고*

</details>
