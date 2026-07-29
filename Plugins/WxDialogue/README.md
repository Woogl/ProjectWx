# WxDialogue — 대화 시스템

> NPC·기믹과 플레이어 사이의 대화를 진행한다. DataTable 로 정의한 대사 사슬을 노드 단위로 순회하며, 표시(UI)와 의미 해석(퀘스트)은 다른 도메인에 넘긴다.

## 책임
**담당**
- 대화 데이터 모델: 노드 = 대사 한 줄 = 행, 대화 1편 = 테이블 1개 (`FWxDialogueTableRow`)
- 세션 진행: 대사 넘기기·선형 진행·종료를 PlayerController 측 세션이 소유 (`UWxDialogueSessionComponent`)
- 대화 정의 부착: 대화 가능한 액터에 시작 노드를 얹는 컴포넌트 (`UWxDialogueComponent`)
- 상호작용 진입점: 상호작용 시 자기 대화 정의를 상호작용자 세션에 넘기는 NPC 베이스 (`AWxNpc`)
- StateTree 관찰 노드: 지정 대상과의 대화 완주를 판정하는 태스크 제공 (`FWxStateTreeTask_WaitDialogueCompleted`)

**경계 (비담당)**
- 대화 위젯 표시 — 시작/대사/종료를 델리게이트로만 발행하고, 구독자([[WxUI]]·뷰모델)가 위젯을 잇는다
- 대화의 의미 판정(수주·완료 등) — 태스크를 놓는 [[WxQuest]] 데이터의 몫. 대화 자체는 뜻을 해석하지 않는다
- 상호작용 스캔·발동·차단 태그 — 계약 인터페이스와 어빌리티는 [[WxCore]]/[[WxCombat]] 영역

## 의존성
- **주요 의존**: [[WxCore]], GameplayAbilities(세션 중 `State.Dialogue` Loose 태그 부착), StateTree(관찰 태스크), GameplayTags, UniversalObjectLocator(`FWxActorTarget` 로케이터)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxDialogueSessionComponent` | PC 소유 세션. Start/Advance 로 진행을 몰고 델리게이트로 발행 | `Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `UWxDialogueComponent` | 대화 가능 액터에 붙는 정의 컴포넌트. 시작 노드만 보유 | `Source/WxDialogue/Public/WxDialogueComponent.h` |
| `AWxNpc` | 대화 NPC 베이스. 상호작용을 세션 시작으로 위임 (Abstract) | `Source/WxDialogue/Public/WxNpc.h` |
| `FWxDialogueTableRow` | 대화 노드 데이터. 대사 한 줄(Speaker/Line) + NextDialogue 로 사슬 구성 | `Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `FWxStateTreeTask_WaitDialogueCompleted` | 지정 대상과의 대화 완주를 관찰로 판정하는 StateTree 태스크 | `Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` |

## 확장 포인트 / 규약
- **새 대화**: `FWxDialogueTableRow` 로우 타입 DataTable 을 만들고, 액터의 `UWxDialogueComponent::StartRow` 에 시작 노드를 지정한다. 진행은 `NextDialogue` 로 이어가고 None 이면 종료다. 분기(선택지)는 없다 — 필요해지면 그때 설계한다.
- **새 NPC**: `AWxNpc` 를 상속(Abstract)하고 인스턴스별 `StartRow`·`NpcName` 을 채운다. 상호작용 계약은 `IWxInteractable`(WxCore) 로 이미 구현되어 있다.
- **퀘스트 연동**: 대화 완주 게이트가 필요하면 퀘스트 StateTree 에 `Wait Dialogue Completed` 태스크를 놓고 `FWxActorTarget` 로 대상 배치 액터를 지정한다. 진입 이전 대화는 세지 않는 엣지 감지다.
- **리플리케이션**: 서버 상호작용 응답 → `StartDialogue` → 소유 클라 `ClientStartDialogue`(Client RPC). 세션 상태는 표시 전용 로컬 상태로 소유 클라가 소유하며 서버 검증은 없다(v1 싱글/리슨 호스트 전제).

## 여기서부터 읽어라
1. `Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 세션 소유·발행·리플리케이션 경계가 헤더 주석에 다 설명돼 있다. 이 모듈의 중심.
2. `Source/WxDialogue/Public/WxDialogueTableRow.h` — 대화가 어떻게 표현되는지(노드=대사 한 줄=행)를 먼저 잡아야 세션 순회가 읽힌다.
3. `Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — EnterRow/Advance 실제 순회와 `State.Dialogue` 태그 부착/해제.

## 관련
- 상위: 위젯을 잇는 [[WxUI]], 완주 태스크를 소비하는 [[WxQuest]], 상호작용·태그 계약을 주는 [[WxCore]]/[[WxCombat]]

---
*문서 기준 커밋 `1bd11a9` · 생성일 2026-07-26 · 소스 10파일 — `/readme-writer`로 갱신*
