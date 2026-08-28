# WxQuest — 퀘스트 시스템

> StateTree 에셋 하나를 퀘스트 하나로 실행하고, 제목·목표 저널을 서버 권위로 관리하는 플러그인. 활성 퀘스트는 동시 1개이며 새 시작은 기존 퀘스트를 교체한다.

## 책임
**담당**
- GameState 부착 컴포넌트가 순정 `UStateTreeComponent` 러너를 권위 측에서 런타임 생성해 소유하고 퀘스트 StateTree 실행을 위임
- 저널(제목·핸들 기반 목표 목록) 상태 관리와 변경 통지 브로드캐스트
- 퀘스트 저작용 StateTree 태스크 노드 제공(제목·목표·이동 대기·체인)
- 레벨 배치 트리거에서 퀘스트를 수주하는 BlueprintCallable 진입점

**경계 (비담당)**
- HUD 표시·뷰모델: `OnJournalChanged` 를 구독해 값을 pull 하는 쪽 → [[WxUI]]
- 보상 지급 등 저널 밖 부수효과는 별도 크로스모듈 StateTree 노드가 담당

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착, 러너 소유·저널 관리의 본체 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 월드 GameState 의 퀘스트 컴포넌트를 찾아 `StartQuest` 위임 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `FWxStateTreeTask_SetQuestTitle` | 진입 시 저널을 새 제목으로 등록(목표 비움) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명 동안 목표 하나를 걸고 이탈 시 걷어감 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어 폰이 로케이터 대상 반경 도달까지 Running 대기 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트 시작을 다음 틱에 예약(체인) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |

## 확장 포인트 / 규약
- 새 퀘스트 태스크: `FStateTreeTaskCommonBase` 파생 USTRUCT + 별도 InstanceData 구조체. `GetInstanceDataType()` 은 헤더 인라인(코딩 규칙 6 예외, 각 헤더에 사유 주석). 오너 GameState 에서 `UWxQuestComponent` 를 찾아 위임하고, 없으면 잘못된 조립으로 보고 경고 후 노옵.
- 저널을 변경하는 태스크(제목·목표)는 완료 판정에서 빠져 있어 진입 즉시 Succeeded 여도 상태를 끝내지 않는다. 상태 완료는 짝이 되는 Wait 태스크가 낸다.
- 저널 정리는 태스크가 아니라 러너의 `OnStateTreeRunStatusChanged`(Running 이탈) 한 곳으로 수렴 — 완료·실패·교체 세 종료 경로를 모두 커버.
- 러너 실행 콜스택 안에서의 퀘스트 교체는 엔진 재진입 가드에 막히므로 `RequestActivateQuest`(다음 틱 예약)를 쓴다. 콜스택 밖에서만 `ActivateQuest` 직접 호출.
- 데이터 주도: 컴포넌트는 어떤 퀘스트 에셋도 알지 않는다. 수주는 트리거 볼륨이 넘기는 에셋, 체인은 `StartNextQuest` 의 소프트 참조로 지정. 부착은 코드가 아니라 Experience 에셋의 컴포넌트 주입 목록.

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 클래스 주석에 러너 소유·권위·저널 수렴 설계가 응축돼 있음
2. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 정지→교체→시작 순서와 다음 틱 예약, 권위 한정 러너 생성의 실제 흐름
3. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` — 목표 수명=상태 수명 규약을 이해하는 대표 태스크

## 관련
- 상위: [[WxCore]] (유일한 Wx 의존)

---
*문서 기준 커밋 `718b827` · 생성일 2026-08-26 · 소스 15파일 — `/readme-writer`로 갱신*
