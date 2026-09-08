# WxQuest — 퀘스트 시스템

> StateTree 에셋 1개를 퀘스트 1개로 실행하고, 서버 권위로 저널(제목·목표)을 관리하는 데이터 주도 퀘스트 시스템. 퀘스트 흐름은 전부 StateTree 노드로 조립한다.

## 책임
**담당**
- 퀘스트 StateTree 실행 및 활성 퀘스트 교체(동시 1개, 권위 전용 러너)
- 저널 상태(제목·목표 목록) 보관과 변경 통지
- 퀘스트 조립용 StateTree 태스크 노드(제목/목표/체인/도달 대기) 제공

**경계 (비담당)**
- 저널 UI 표시는 [[WxUI]]로 위임(본 모듈은 `OnJournalChanged` 통지만, 뷰모델이 pull)
- 보상 지급·월드 부수효과는 각 도메인 StateTree 노드로 위임(본 모듈은 실행 진입점만)
- 퀘스트 에셋(무엇을 실행할지)은 데이터·레벨 배치가 지정, 본 모듈은 에셋 불가지

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착. 권위 러너 소유·저널 관리의 중심 | `Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 월드 GameState의 퀘스트 컴포넌트를 찾아 `StartQuest` 위임(레벨 트리거용) | `Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `FWxStateTreeTask_SetQuestTitle` | 저널 제목 등록 태스크 | `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명과 함께 목표를 걸고 걷어가는 태스크 | `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어가 대상 반경 도달까지 대기(상태 완료를 내는 짝) | `Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트를 다음 틱에 예약해 퀘스트 체인 구성 | `Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |

## 확장 포인트 / 규약
- 새 퀘스트 = `UStateTree` 에셋 1개. 수주는 레벨 트리거가 `UWxQuestLibrary::StartQuest`로, 체인은 `StartNextQuest` 태스크의 `Quest` 소프트 참조로 지정한다.
- 새 목표 종류: `FStateTreeTaskCommonBase` 파생 태스크를 추가하고, 컨텍스트 오너(GameState)에서 `UWxQuestComponent`를 찾아 저널 갱신을 요청한다. 컴포넌트가 없으면 잘못된 조립이므로 경고 후 무시하는 것이 관례.
- StateTree 규약: 목표/제목 태스크는 완료 판정에서 빠져 있어 진입 즉시 Succeeded 여도 상태를 끝내지 않는다 — 상태 완료는 짝이 되는 `WaitMoveToTarget` 등 대기 태스크가 낸다.
- 저널 정리는 태스크가 아니라 러너의 `HandleStateTreeRunStatusChanged`(완료·실패·교체가 수렴)에서 한다. 러너 실행 콜스택 안에서의 재활성화는 엔진 재진입 가드에 막히므로 `RequestActivateQuest`로 다음 틱 예약한다.

## 여기서부터 읽어라
1. `Source/WxQuest/Public/Quest/WxQuestComponent.h` — 러너 소유·권위 전용 실행·저널 정리 수렴 등 시스템 설계 근거가 클래스 doc-comment에 집약
2. `Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 권위 판정과 러너 런타임 생성, 예약 활성화 흐름의 실제 구현
3. `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` — 목표 수명 == 상태 수명 패턴의 대표 예

## 관련
- 상위: [[WxCore]] (공용 정의 · GameState 부착 지점)
- 표시: [[WxUI]] (저널 뷰모델)

---
*문서 기준 커밋 `ba86cff` · 생성일 2026-09-08 · 소스 14파일 — `/readme-writer`로 갱신*
