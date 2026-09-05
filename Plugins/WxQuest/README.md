# WxQuest — 퀘스트 시스템

> StateTree 에셋 1개를 퀘스트 1개로 보고, GameState에 붙은 컴포넌트가 서버 권위로 실행·저널(제목·목표)을 관리한다. 무엇을 실행할지는 전부 데이터(에셋)가 지정한다.

## 책임
**담당**
- 퀘스트 StateTree 러너의 권위 측 구동과 수명 관리 (활성 퀘스트 동시 1개, 새 시작은 교체)
- 저널 상태 보관 — 제목 1개 + 목표(발급 핸들로 식별) 목록, 종료 3경로(완료·실패·교체)의 정리 수렴
- 퀘스트 저작용 StateTree 태스크 팔레트 (제목·목표 설정, 다음 퀘스트 체인, 지점 도달 대기)
- 외부(트리거 볼륨 등)에서 퀘스트를 수주시키는 BP 진입점

**경계 (비담당)**
- 컴포넌트 부착: 코드가 아니라 GameMode가 고른 Experience 에셋의 주입 목록으로 붙는다 (WxGame/GameFeature 측)
- 저널 표시: `OnJournalChanged`를 구독해 pull 하는 HUD 뷰모델은 [[WxUI]] 몫
- 보상 지급 등 크로스모듈 부수효과 노드(GiveRewards 류)는 이 모듈 밖

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착. 권위에서 순정 러너를 런타임 생성·소유하고 저널을 보관 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | 월드 GameState의 컴포넌트를 찾아 `StartQuest`로 위임하는 BP 수주 진입점 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `FWxStateTreeTask_SetQuestTitle` | 진입 시 저널 제목 등록(목표 비움). 상태를 끝내지 않음 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 진입 시 목표 1개 걸고 상태 이탈 시 핸들로 걷어감 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트를 다음 틱으로 예약하고 즉시 Succeeded (체인) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_StartNextQuest.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 0번 컨트롤러 폰이 로케이터 대상 반경에 들 때까지 Running으로 상태 완료를 냄 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |

## 확장 포인트 / 규약
- 새 퀘스트 태스크: `FStateTreeTaskCommonBase` 상속 + `FInstanceDataType`/`GetInstanceDataType()` 규약(엔진 StateTree와 동형). 컴포넌트를 못 찾으면 "러너 밖 오조립"으로 보고 경고 또는 Failed 처리하는 것이 이 모듈 태스크의 공통 관례.
- 목표는 문구가 아니라 발급 핸들로 지목한다(같은 문구가 둘일 수 있음). `SetQuestObjective` 태스크는 목표 수명을 상태 수명에 묶어, 별도 정리 태스크 없이 ExitState에서 걷어간다.
- 저널 정리는 태스크가 아니라 러너의 `EStateTreeRunStatus` 변경 통지 한 곳으로 수렴 — 완료·실패·교체가 모두 여기로 온다.
- 리플리케이션/권한: 러너는 권위(싱글/리슨 호스트)에서만 뜬다. Experience 주입 목록엔 사이드 구분이 없어 클라 GameState에도 컴포넌트 사본이 붙으므로, 러너를 권위에서만 띄우는 것은 컴포넌트 자신의 책임. 러너 콜스택 안 활성화 요청은 재진입 가드에 막혀 다음 틱 예약으로 우회한다(`RequestActivateQuest`).
- 배치 대상 지정: `FWxStateTreeTask_WaitMoveToTarget`은 `FUniversalObjectLocator`로 레벨 액터를 직접 가리킨다(ST 컴파일러 레벨 참조 검증 회피, WP 언로드/재로드 자연 처리).

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 모듈 전체 설계(에셋 불가지·권위 전용 러너·저널 수렴)가 헤더 주석에 응축돼 있다.
2. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 러너 생성·교체 거부·다음 틱 예약·저널 정리의 실제 제어 흐름.
3. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` — 외부에서 퀘스트가 들어오는 단일 수주 경로.

## 관련
- 상위: 컴포넌트 부착·수주 트리거는 Experience/GameFeature 층([[WxGame]]), 저널 UI는 [[WxUI]]. 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `a1df17d` · 생성일 2026-09-04 · 소스 14파일 — `/readme-writer`로 갱신*
