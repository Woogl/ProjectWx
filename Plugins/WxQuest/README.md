# WxQuest — 퀘스트 시스템

> StateTree 에셋 1개를 퀘스트 1개로 보고, GameState에 붙은 컴포넌트가 서버 권위로 실행·저널(제목·목표)을 관리하는 데이터 주도 퀘스트 시스템.

## 책임
**담당**
- 활성 퀘스트(동시 1개) StateTree 실행을 권위 측에서만 구동 — 비-권위 머신에선 러너를 띄우지 않는다.
- 저널 상태(제목 1개 + 핸들로 식별되는 목표 목록) 보관과 `OnJournalChanged` 통지.
- 퀘스트 진행을 기술하는 StateTree 태스크 노드 제공(제목/목표 설정, 다음 퀘스트 체인, 목표 지점 도달 대기).
- 수주 진입점(`UWxQuestLibrary::StartQuest`) — 레벨 배치 트리거 등에서 GameState 컴포넌트로 위임.

**경계 (비담당)**
- 저널의 화면 표시 — HUD 뷰모델이 `OnJournalChanged`를 구독해 pull ([[WxUI]]).
- 컴포넌트 부착 — 코드가 아니라 GameMode가 고른 Experience 에셋의 주입 목록이 붙인다 ([[WxGame]]).
- 퀘스트별 부수효과(스폰·보상 등) — 각 퀘스트의 StateTree 에셋과 크로스모듈 태스크가 지정.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착, 러너 소유·저널 관리의 중심. 모든 태스크가 오너에서 이걸 찾아 위임한다 | `Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 월드→GameState 컴포넌트 위임하는 수주 진입점(BlueprintCallable) | `Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `FWxStateTreeTask_SetQuestTitle` | 상태 진입 시 저널 제목 등록 | `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명 동안 목표 1개 유지(진입 시 걸고 이탈 시 걷음) | `Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어 폰이 대상 반경 도달할 때까지 Running으로 상태 완료를 낸다 | `Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트를 다음 틱 예약(체인) | `Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |

## 확장 포인트 / 규약
- 새 퀘스트 = 새 `UStateTree` 에셋. 컴포넌트는 에셋 불가지 — 실행 대상은 트리거의 `StartQuest` 인자 또는 `StartNextQuest`의 소프트 참조로만 지정한다.
- 새 태스크 노드는 `FStateTreeTaskCommonBase` 상속 + 짝 `...InstanceData` USTRUCT. `GetInstanceDataType()`은 헤더 인라인(코딩 규칙 6 예외, 사유 주석 있음).
- 상태 완료 규약: Set계열 태스크는 완료 판정에서 빠져 진입 즉시 Succeeded여도 상태를 끝내지 않는다 — 상태 완료는 짝이 되는 Wait 태스크가 낸다.
- 러너 실행 콜스택 안에서 에셋 교체는 거부되므로, 콜스택 내 활성화는 `RequestActivateQuest`로 다음 틱에 미룬다(`ActivateQuest`는 콜스택 밖 전용).
- 저널 정리는 태스크가 아니라 러너의 `RunStatus` 변경 통지 한 곳(완료·실패·교체 수렴)에서 한다.

## 여기서부터 읽어라
1. `Source/WxQuest/Public/Quest/WxQuestComponent.h` — 클래스 doc-comment에 권위 모델·에셋 불가지·저널 수명 규약이 응축돼 있다.
2. `Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 러너 런타임 생성·위임·다음 틱 예약의 실제 구현.
3. `Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` — 상태 완료를 내는 대기 태스크와 UniversalObjectLocator 대상 지정 패턴.

## 관련
- 상위: 컴포넌트 부착은 [[WxGame]]의 Experience 주입, 저널 표시는 [[WxUI]] 뷰모델. 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `27fb65d` · 생성일 2026-09-02 · 소스 13파일 — `/readme-writer`로 갱신*
