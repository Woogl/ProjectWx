# WxQuest — 퀘스트 시스템

> 퀘스트 1개를 UStateTree 에셋 1개로 표현하고, 그 실행과 저널(제목·목표)을 GameState에 붙은 컴포넌트가 서버 권위로 관리한다. 수주·목표·체인은 전부 데이터(에셋·StateTree 태스크)가 지정한다.

## 책임
**담당**
- 활성 퀘스트 하나의 StateTree 실행(권위 측 러너 소유·구동)과 교체
- 저널 상태 보관: 제목 1개 + 목표 여러 개(발급 핸들 기준), 변경 통지(`OnJournalChanged`)
- StateTree 태스크 팔레트: 제목/목표 설정, 다음 퀘스트 체인, 목표 지점 도달 대기
- 저널 진입점 위임: 레벨 배치물(트리거 볼륨 등)이 부르는 `UWxQuestLibrary::StartQuest`

**경계 (비담당)**
- 퀘스트 저널의 화면 표시 — HUD 뷰모델이 `OnJournalChanged`를 구독해 pull ([[WxUI]])
- 퀘스트 컴포넌트의 부착 — 코드가 아니라 GameMode가 고른 Experience 에셋의 주입 목록 ([[WxCore]])
- 보상 지급·스폰 등 월드 부수효과 — 개별 StateTree 태스크(예: GiveRewards)가 담당, 본 모듈은 흐름만 구동

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착. 러너 소유·저널 보관의 중심 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 레벨 배치물이 부르는 수주 진입점(GameState 컴포넌트로 위임) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `FWxStateTreeTask_SetQuestTitle` | 진입 시 저널 제목 등록(목표 비움) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명 = 목표 수명. 진입 시 걸고 이탈 시 걷음 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 대상 반경 도달까지 대기(상태 완료를 내는 짝) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트를 다음 틱 예약(체인) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |

## 확장 포인트 / 규약
- 새 퀘스트 스텝 타입: `FStateTreeTaskCommonBase` 상속 USTRUCT로 태스크를 추가하고, 오너(GameState)에서 `UWxQuestComponent`를 찾아 위임한다. 태스크는 퀘스트 에셋을 몰라야 한다(에셋 불가지).
- "설정" 태스크(제목·목표)는 완료 판정에서 빠져 진입 즉시 Succeeded여도 상태를 끝내지 않는다 — 상태 완료는 짝이 되는 Wait 태스크가 낸다. 이 짝 구조를 유지할 것.
- 러너 실행 콜스택 안에서 에셋 교체는 거부되므로, 태스크 안에서의 활성화는 `RequestActivateQuest`(다음 틱 예약)로만 한다. 저널 정리는 태스크가 아니라 러너의 실행 상태 변경(`HandleStateTreeRunStatusChanged`)에서 일괄 처리 — 완료·실패·교체 세 종료 경로가 한 곳으로 수렴한다.
- 데이터 주도: 퀘스트=UStateTree 에셋, 수주=레벨 트리거가 넘기는 에셋, 체인=`StartNextQuest`의 소프트 참조, 도달 대상=`FUniversalObjectLocator`(배치 액터 직접 지정).
- 리플리케이션/권한: 러너는 권위(싱글/리슨 호스트)에만 생성되고 비-권위 머신에선 `QuestStateTree`가 null. 부수효과 단일 구동과 저널 채움은 권위 전용. 배치물·태스크의 0번 컨트롤러 전제는 v1 싱글/리슨 호스트 기준.

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 헤더 주석이 러너 소유·권위 모델·저널 정리 수렴을 모두 설명하는 시스템 지도
2. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 러너 런타임 생성·활성화 교체·다음 틱 예약의 실제 흐름
3. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` — 태스크 한 종을 끝까지 보며 EnterState/Tick과 로케이터 해석 규약을 익히는 표본

## 관련
- 상위: 저널 표시는 [[WxUI]] HUD 뷰모델, 컴포넌트 부착은 [[WxCore]] Experience 주입 목록

---
*문서 기준 커밋 `bb06a17` · 생성일 2026-08-30 · 소스 14파일 — `/readme-writer`로 갱신*
