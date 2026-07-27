# WxQuest — 퀘스트 시스템

> StateTree 에셋 1개를 퀘스트 1개로 삼아, GameState 컴포넌트가 서버 권위로 실행하고 저널(제목·목표)을 관리하는 데이터 주도 퀘스트 시스템. 활성 퀘스트는 동시 1개이며, 자동 탑재·체인·외부 호출로 시작된다.

## 책임
**담당**
- 퀘스트 StateTree 러너의 소유·실행(권위 측 런타임 생성, `UStateTreeComponent` 위임)
- 퀘스트 저널(제목 1개 + 목표 목록) 서버 권위 관리 및 변경 통지
- 퀘스트 시작 경로 조정: 자동 탑재(`bAutoStart` 에셋 레지스트리 발견)·체인(`StartNextQuest`)·외부 호출(Library)
- 퀘스트 전용 StateTree 노드(제목 등록·목표 표시·타겟 도달 대기·다음 퀘스트 예약) 제공

**경계 (비담당)**
- 저널 표시(HUD 뷰모델·위젯) — [[WxUI]]
- 대화·트리거로부터의 퀘스트 시작 트리거 소유(레벨 스크립트·볼륨)는 외부, 본 모듈은 진입점만 노출

## 의존성
- **주요 의존**: [[WxCore]] (`FWxActorTarget` 등), StateTree / GameplayStateTree, ModularGameplay(`UGameStateComponent`), GameplayTags, UniversalObjectLocator
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxQuestComponent` | GameState 부착, 러너 소유·저널 관리의 중심 | `Source/WxQuest/Public/Quest/WxQuestComponent.h` |
| `UWxQuestLibrary` | Blueprint 진입점(StartQuest·SendQuestEvent) | `Source/WxQuest/Public/Quest/WxQuestLibrary.h` |
| `UWxQuestStateTree` | 퀘스트 1개를 담는 전용 ST 에셋 타입(`bAutoStart`) | `Source/WxQuest/Public/Quest/WxQuestStateTree.h` |
| `FWxStateTreeTask_SetQuestTitle` | 저널 등록 태스크(퀘스트당 1회) | `Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `FWxStateTreeTask_SetQuestObjective` | 상태 수명 동안 목표 하나를 표시하는 태스크 | `Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `FWxStateTreeTask_WaitMoveToTarget` | 플레이어 폰의 대상 반경 도달 대기 태스크 | `Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `FWxStateTreeTask_StartNextQuest` | 다음 퀘스트 시작 예약 태스크(체인) | `Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` |
| `FWxOnQuestJournalChanged` | 저널 변경 통지 델리게이트(HUD 구독용) | `Source/WxQuest/Public/Quest/WxQuestComponent.h` |

## 확장 포인트 / 규약
- **새 퀘스트 노드**: `FStateTreeTaskCommonBase` 를 상속한 `FWxStateTreeTask_*` 구조체 + `*InstanceData` 쌍으로 추가. 노드는 컨텍스트 오너(GameState)에서 `UWxQuestComponent` 를 찾아 위임하며, 컴포넌트 부재는 잘못된 조립으로 보아 `Failed`. 기믹 노드의 권위 게이트·초기 진입 스킵 게이트는 복사하지 말 것(러너가 권위에만 존재).
- **저널 표시 규약**: 제목은 퀘스트당 하나이므로 진행이 시작되는 상태에 `SetQuestTitle` 을 한 번만 둔다. 목표는 `SetQuestObjective` 가 상태에 들 때 걸고 날 때 걷어가므로 상태 수명이 곧 목표 수명이며, 부모 상태와 자식 상태에 각각 걸면 둘이 동시에 표시된다. 두 노드 모두 상태 완료 판정에서 빠져 있다 — 판정에 끼면 자식을 둔 상태를 즉시 완료시켜 퀘스트가 관통된다.
- **레벨 액터 지정**: `FWxActorTarget`(WxCore) 로 배치 액터를 직접 지정 — 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증을 통과하고 WP/PIE 해석이 엔진 내장. 해석은 매 틱 `SyncFind`(강제 로드 없음).
- **새 퀘스트 에셋**: 스키마 `StateTreeComponentSchema` 로 고정된 `UWxQuestStateTree` 를 WxEditor 팩토리로 생성. 자동 시작 퀘스트는 `bAutoStart` 를 켜되 프로젝트에 1개만(활성 1개 원칙), 값 변경은 에셋 재저장 후 발견 반영.
- **부착**: 코드가 아니라 GameMode 가 고른 `Experience` 에셋의 주입 설정으로 GameState 에 부착(GameState 는 본 클래스를 모름).
- **재진입 규약**: 러너 실행 콜스택 안 시작은 `RequestStartQuest`(다음 틱 예약), 밖은 `StartQuest`(즉시 교체). 저널 정리는 노드가 아니라 러너 실행 상태 변경 통지(`HandleStateTreeRunStatusChanged`)로 수렴.

## 여기서부터 읽어라
1. `Source/WxQuest/Public/Quest/WxQuestComponent.h` — 실행·저널·시작 경로가 모이는 중심. 헤더 주석이 설계 전체를 설명한다.
2. `Source/WxQuest/Public/Quest/WxQuestStateTreeNodes.h` — 데이터(ST)가 무엇을 실행하는지 규정하는 노드 4종.
3. `Source/WxQuest/Private/Quest/WxQuestComponent.cpp` — 러너 런타임 생성·탑재 발견·재진입 처리의 실제 구현.

## 관련
- 상위: [[WxCore]] (`FWxActorTarget`), [[WxUI]] (저널 표시)

---
*문서 기준 커밋 `1bd11a9` · 생성일 2026-07-26 · 소스 9파일 — `/readme-writer`로 갱신*
