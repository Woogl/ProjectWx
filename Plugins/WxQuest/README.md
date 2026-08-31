# WxQuest — 퀘스트 시스템

> StateTree 하나를 퀘스트 하나로 삼아, GameState 에 붙은 컴포넌트가 서버 권위로 실행하고 저널(제목·목표)을 관리한다. 퀘스트 흐름은 전부 데이터(StateTree 태스크)로 조립한다.

## 책임
**담당**
- 활성 퀘스트 1개의 StateTree 러너를 권위 측에서 생성·구동하고 교체·체인을 조율
- 저널(제목 1개 + 목표 N개) 상태를 발급 핸들 기준으로 유지·정리하고 변경을 브로드캐스트
- 퀘스트 저작용 StateTree Task 노드(제목/목표 설정, 다음 퀘스트 시작, 목표 지점 도달 대기) 제공
- 레벨 배치물(트리거 볼륨 등)이 부를 수주 진입점(`UWxQuestLibrary::StartQuest`)

**경계 (비담당)**
- 저널 표시(HUD·뷰모델)는 델리게이트 구독자에게 위임 — [[WxUI]]
- 보상 지급 등 퀘스트 외 부수효과는 별도 태스크/모듈(예: GiveRewards)에 위임

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착. 러너 소유·저널 관리·체인 조율의 중심 | `Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 월드에서 퀘스트 컴포넌트를 찾아 `StartQuest` 위임(BP 수주 진입점) | `Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `FWxStateTreeTask_SetQuestTitle` | 진입 시 저널 제목 등록(목표 비움) | `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명 = 목표 수명. 진입 시 걸고 이탈 시 걷어감 | `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어 폰이 대상 반경 도달까지 대기 후 상태 완료 | `Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트를 다음 틱에 예약(체인) | `Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |
| `FWxOnQuestJournalChanged` | 저널 변경 델리게이트. 뷰모델이 구독해 현재 값 pull | `Source/WxQuest/Public/Quest/WxQuestComponent.h` |

## 확장 포인트 / 규약
- **퀘스트 = UStateTree 에셋 1개**, 동시 활성 1개(새 시작은 기존을 정지·교체). 러너는 권위(서버/리슨 호스트)에만 존재하므로 저널·부수효과가 단일 구동된다.
- **새 목표 유형**은 `FStateTreeTaskCommonBase` 파생 Task 로 추가한다. 「설정 태스크(즉시 Succeeded, 상태를 끝내지 않음)」와 「대기 태스크(조건 충족 시 상태 완료)」를 짝지어 한 상태에 배치하는 것이 관례다. 상태 완료 신호는 대기 태스크가 낸다.
- **저널 정리는 태스크가 아니라 러너의 실행 상태 변경 통지**로 수렴한다 — 완료·실패·교체 세 종료 경로가 한 콜백(`HandleStateTreeRunStatusChanged`)으로 모인다.
- **러너 콜스택 안 재진입 금지**: 태스크에서의 활성화·체인은 즉시 교체가 거부되므로 다음 틱 예약(`RequestActivateQuest`)으로 처리한다.
- **컴포넌트 부착은 코드가 아니라 Experience 에셋의 주입 목록**으로 한다(GameState 는 본 클래스를 모름). 목록에 사이드 구분이 없어 클라 사본도 붙으므로, 러너를 권위에서만 띄우는 것은 컴포넌트 책임.
- **대상 지정은 `FUniversalObjectLocator`** 로 배치 액터를 직접 참조 — ST 컴파일러의 레벨 액터 참조 검증을 우회하고 WP/PIE 해석을 엔진에 위임한다.

## 여기서부터 읽어라
1. `Source/WxQuest/Public/Quest/WxQuestComponent.h` — 러너 소유·저널·체인의 설계 근거가 헤더 주석에 응축돼 있다
2. `Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 권위 판정, 다음 틱 예약, RunStatus 콜백의 실제 흐름
3. `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` — 설정/대기 태스크 짝짓기 규약의 대표 예

## 관련
- 상위: 레벨 배치 트리거 볼륨·BP 가 `UWxQuestLibrary::StartQuest` 로 수주. Experience 에셋이 컴포넌트를 GameState 에 주입. 저널은 [[WxUI]] 뷰모델이 델리게이트로 구독.

---
*문서 기준 커밋 `b3f982b` · 생성일 2026-08-31 · 소스 14파일 — `/readme-writer`로 갱신*
