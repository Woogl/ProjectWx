# WxQuest — 퀘스트 시스템

> StateTree 에셋 1개 = 퀘스트 1개로 삼아, GameState 에 붙은 서버 권위 컴포넌트가 러너를 돌리고 저널(제목·목표)을 관리한다. 컴포넌트는 어떤 퀘스트 에셋도 알지 않으며 무엇을 실행할지는 전부 데이터가 지정한다.

## 책임
**담당**
- 퀘스트 실행 수명주기: 활성 퀘스트 1개를 권위에서 실행, 시작/교체/이벤트 전달, 완료·실패·교체의 저널 정리 수렴
- 저널 상태: 제목 1개 + 목표 N개(발급 핸들로 지목)를 권위에서 보관하고 변경 통지(`OnJournalChanged`) 발화
- 퀘스트 저작 프리미티브: 퀘스트 전용 StateTree 에셋 타입과 저작용 StateTree Task 세트(제목·목표·체인·도착 대기)
- 레벨 진입점: 배치 트리거에서 호출하는 Blueprint 라이브러리(활성화·이벤트 전송)

**경계 (비담당)**
- 저널의 시각화(HUD·저널 위젯) — 뷰모델이 컴포넌트를 구독해 pull
- 컴포넌트 부착 자체 — GameMode 가 고른 Experience 에셋의 주입 목록이 담당(GameState 는 본 클래스를 모름)
- 퀘스트 에셋 신규 생성 팩토리 — [[WxToolset]] / WxEditor 측
- 보상 지급 등 도메인 부수효과 노드 — 각 도메인 모듈이 제공하는 별도 ST 노드

## 의존성
- **주요 의존**: `WxCore`. 엔진 서브시스템으로 StateTree / GameplayStateTree(컴포넌트 러너·태스크 베이스), ModularGameplay(`UGameStateComponent`), UniversalObjectLocator(도착 대상 지정)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착. 권위 러너 소유·저널 관리의 중심. 모든 태스크가 여기로 위임 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestStateTree` | 퀘스트 1개를 담는 전용 ST 에셋 타입. 지정 필드·API 가 이 타입만 받아 오지정 차단 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestStateTree.h` |
| `UWxQuestLibrary` | 배치 트리거용 BP 진입점(`ActivateQuest`·`SendQuestEvent`), GameState 컴포넌트로 위임 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `FWxStateTreeTask_SetQuestTitle` | 진입 상태에 한 번 걸어 저널 제목 등록, 완료 없이 머묾 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestTitle.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명 = 목표 수명. 진입 시 걸고 이탈 시 걷어감(핸들 기반) | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어 폰이 지정 대상 반경 도달까지 대기 후 완료. 대상은 UOL 배열 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_WaitMoveToTarget.h` |
| `FWxStateTreeTask_ActivateNextQuest` | 다음 퀘스트를 다음 틱 예약하고 즉시 Succeeded. 소프트 참조로 체인 구성 | `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_ActivateNextQuest.h` |

## 확장 포인트 / 규약
- **퀘스트 저작 = ST 그래프 조립**: 퀘스트는 코드가 아니라 `UWxQuestStateTree` 에셋 하나로 저작한다. 스키마는 `StateTreeComponentSchema` 로 고정(컴포넌트 러너 전제).
- **저널 표시 규약**: 제목은 진행이 시작되는 상태에 한 번만(`SetQuestTitle`), 목표는 그 상태의 자식들이 각자 `SetQuestObjective` 로 건다. 병렬 상태가 없어도 부모·자식이 각각 걸면 다중 목표가 동시에 표시된다.
- **완료 판정 함정**: `SetQuestTitle`/`SetQuestObjective` 는 상태에 머무는 Running 태스크이며, 실제 상태 완료는 짝이 되는 대기 태스크(`WaitMoveToTarget` 등)가 낸다. `SetQuestObjective` 는 `bConsideredForCompletion=false` 라 오조립 시에도 트리 진행을 막지 않고 경고 로그만 남긴다.
- **새 목표 조건 태스크 추가**: `FStateTreeTaskCommonBase` 를 상속해 `EnterState`/`Tick` 에서 조건을 판정, 컨텍스트 오너(GameState)에서 `UWxQuestComponent` 를 찾아 위임하는 패턴을 따른다(`WaitMoveToTarget` 참고).
- **레벨 참조는 UOL 로**: 배치 액터 지정은 `FUniversalObjectLocator`(순수 구조체) 배열을 쓴다 — ST 컴파일러의 레벨 액터 참조 검증을 우회하고 WP/PIE 해석이 내장된다. 5.8 에디터 제한으로 단일 대상도 배열로 받는다.
- **권위 전제**: 러너는 권위(싱글/리슨 호스트)에만 존재한다. 비-권위 GameState 에도 컴포넌트 사본이 붙으므로 러너를 권위에서만 띄우는 것이 컴포넌트 책임이다. 태스크의 0번 컨트롤러 사용도 v1 싱글/리슨 호스트 전제.

## 여기서부터 읽어라
1. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h` — 클래스 doc-comment 가 시스템 전체 설계(러너 위임·저널 정리 수렴·에셋 불가지)를 담은 지도다
2. `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 활성화·재진입 방어(다음 틱 예약)·러너 상태 콜백의 실제 구현
3. `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxStateTreeTask_SetQuestObjective.h` — 목표=상태 수명 규약과 완료 판정 함정을 이해하는 출발점

## 관련
- 상위: `WxGame` Experience 에셋이 컴포넌트 주입을 결정 · 에셋 팩토리는 [[WxToolset]] / WxEditor

---
*문서 기준 커밋 `dfd2174` · 생성일 2026-08-12 · 소스 15파일 — `/readme-writer`로 갱신*
