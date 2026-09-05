# WxQuest — 퀘스트 시스템

> StateTree 에셋 하나를 퀘스트 하나로 실행하고, 저널(제목·목표)을 서버 권위로 관리한다. "무엇을 실행할지"는 전부 데이터(에셋·태스크)가 지정하며 모듈 자체는 어떤 퀘스트도 알지 않는다.

## 책임
**담당**
- GameState 에 부착되어 퀘스트 StateTree 러너를 권위 측에서만 구동하고 실행을 위임 (`UWxQuestComponent`)
- 저널 상태 보유: 제목 1개 + 목표 다수(발급 핸들 기반), 변경 시 델리게이트 발화
- 퀘스트 수주/교체/체인 진입점 제공 (`UWxQuestLibrary::StartQuest`, StateTree 태스크들)
- 퀘스트 저작에 쓰는 StateTree 태스크 노드 4종 제공

**경계 (비담당)**
- 저널을 화면에 그리는 것 — HUD 뷰모델이 `OnJournalChanged` 를 구독해 pull ([[WxUI]])
- 컴포넌트 부착 — 코드가 아니라 Experience 에셋의 주입 목록이 GameState 에 붙인다 (GameFeature/Experience 계층)
- 보상 지급 등 실제 게임플레이 효과 — 다른 모듈의 StateTree 노드(예: GiveRewards)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | 모듈의 심장. 권위 측 러너 소유·저널 보유, 모든 태스크가 오너에서 찾아 위임 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 외부(트리거 볼륨 등) → 컴포넌트 진입점. `StartQuest` | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `FWxStateTreeTask_SetQuestTitle` | 저널 제목 등록 (상태 완료는 내지 않음) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명 = 목표 수명. 진입 시 걸고 이탈 시 걷어감 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 상태 완료를 내는 대기 태스크. 대상 도달까지 Running | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_StartNextQuest` | 퀘스트 체인. 다음 퀘스트를 다음 틱에 예약 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |

## 확장 포인트 / 규약
- **새 퀘스트 저작**: UStateTree 에셋 1개 = 퀘스트 1개. 활성 퀘스트는 동시 1개(새 시작은 교체). 상태 안에서 Set 태스크로 저널을 채우고, 짝이 되는 Wait 태스크가 상태 완료를 낸다 — Set 계열은 진입 즉시 Succeeded 라도 상태를 끝내지 않는다.
- **새 태스크 추가**: `FStateTreeTaskCommonBase` 상속, `GetInstanceDataType()` 는 헤더 인라인(코딩 규칙 6 예외, 주석 참조). 오너 컨텍스트(GameState)에서 `UWxQuestComponent` 를 찾아 위임하고, 없으면 잘못된 조립으로 보아 경고/Failed 처리한다.
- **리플리케이션/권한**: 러너는 권위(싱글/리슨 호스트)에서만 BeginPlay 에 런타임 생성되고 비-권위 머신에선 null. 저널·월드 부수효과 단일 구동은 이 규약에 의존한다. 러너 실행 콜스택 안에서는 에셋 교체가 거부되므로 태스크발 활성화는 `RequestActivateQuest` 로 다음 틱 예약한다.
- **저널 정리**: 태스크가 아니라 러너의 실행 상태 변경 통지(`HandleStateTreeRunStatusChanged`)로 수렴 — 완료·실패·교체 세 종료 경로가 한 곳으로 모인다.

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 클래스 doc-comment가 러너 소유·권위 모델·에셋 불가지 설계를 통째로 설명한다. 모듈 이해의 출발점.
2. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 러너 생성/위임/저널 정리의 실제 구현.
3. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` — "상태 수명 = 목표 수명" 규약이 왜 정리 태스크를 없애는지.

## 관련
- 상위: 저널을 구독·표시하는 [[WxUI]] HUD, 컴포넌트를 주입하는 Experience 계층(GameFeature), 보상 등 크로스모듈 StateTree 노드

---
*문서 기준 커밋 `f826b21` · 생성일 2026-09-05 · 소스 14파일 — `/readme-writer`로 갱신*
