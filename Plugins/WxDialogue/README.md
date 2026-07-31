# WxDialogue — 대화 시스템

> 데이터 테이블로 정의한 대사를 NPC 상호작용·퀘스트 트리에서 열어, PlayerController에 주입된 세션 컴포넌트가 소유 클라에서 진행·연출하고, 관찰자(퀘스트)가 완주를 판정하는 대화 시스템.

## 책임
**담당**
- 대화 데이터 정의 — `FWxDialogueTableRow`(화자·대사·`NextDialogue` 링크, 노드 = 대사 한 줄 = 행, 대화 1편 = 테이블 1개)
- 대화 가능 액터에 시작 노드만 얹는 정의 컴포넌트 — `UWxDialogueComponent`
- PC에 주입되는 세션의 소유·진행(`Advance`)·종료와 대화 전용 카메라 연출 — `UWxDialogueSessionComponent`
- 대화 NPC 베이스와 상호작용 시 세션으로의 대화 진입 — `AWxNpc`
- 대화를 StateTree에서 관찰·출력하는 크로스모듈 노드 — `WaitDialogueCompleted`·`PlayDialogue`

**경계 (비담당)**
- 대화 창 열고 닫기·위젯·뷰모델 — 세션은 `State.Dialogue` 태그와 델리게이트만 발행, 위임 [[WxUI]]
- 대화의 의미 해석(수주·납품 판정)과 대화를 여는 퀘스트 진행 — 위임 [[WxQuest]]
- 상호작용 스캔·발동·프롬프트 계약(`IWxInteractable`) — 위임 [[WxCore]]

## 의존성
- **주요 의존**: `WxCore`(`IWxInteractable`·`WxGameplayTags::State_Dialogue`), `GameplayAbilities`(세션 중 ASC Loose 태그), `ModularGameplay`(컨트롤러 컴포넌트 주입), `StateTreeModule`(관찰·출력 태스크)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 노드 한 줄(화자·대사·`NextDialogue`). 대화 1편 = 테이블 1개 | `Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `UWxDialogueSessionComponent` | PC에 주입되는 세션. 진입(`StartDialogue`/`StartDialogueRow`)·진행·카메라·태그 (모듈 심장부) | `Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `UWxDialogueComponent` | 대화 가능 액터가 보유하는 대화 정의(시작 행만) | `Source/WxDialogue/Public/WxDialogueComponent.h` |
| `AWxNpc` | 대화 NPC 베이스(Abstract). 메시가 상호작용 영역, 상호작용을 세션에 위임 | `Source/WxDialogue/Public/WxNpc.h` |
| `FWxStateTreeTask_WaitDialogueCompleted` | 지정 대사를 거친 대화 완주를 관찰(퀘스트 게이트) | `Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` |
| `FWxStateTreeTask_PlayDialogue` | 트리가 대사를 열어 연출(독백·무전·처치 후) | `Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` |

## 확장 포인트 / 규약
- **새 대화**: `FWxDialogueTableRow` 로우 타입 DataTable을 만들고 `NextDialogue`로 노드를 잇는다(대사가 비면 종료). 액터의 `UWxDialogueComponent::StartRow`에 시작 노드를 지정. 분기(선택지)는 없다 — 필요해지면 그때 설계.
- **새 NPC**: `AWxNpc`(Abstract)를 상속하고 인스턴스별 `StartRow`·`NpcName`을 채운다. 상호작용 계약은 `IWxInteractable`(WxCore)로 이미 구현됨.
- **대화 진입 두 경로**: 액터 기반은 상호작용 응답이 `StartDialogue(UWxDialogueComponent*)`, 액터 아닌 쪽(퀘스트 ST)은 `StartDialogueRow(RowHandle, Target)`. 서버 권위 진입 → 소유 클라 `ClientStartDialogue`(Client RPC)로 세션 오픈. 세션은 표시 전용 로컬 상태(v1 싱글/리슨 호스트 전제, 서버 검증 없음).
- **퀘스트 연동**: 완주 게이트는 `Wait Dialogue Completed`에 대화 행 지정(시작 행=대화 전체, 중간·끝 행=그 대사까지 읽은 대화). 트리가 대사를 소유해야 하면 `Play Dialogue`에 `StartRow` 지정. 두 노드 모두 0번 컨트롤러 세션을 폴링·관찰.
- **카메라 조정**: 주입 컴포넌트라 세션 헤더의 `Camera*` 기본값이 곧 실제 값. 에셋으로 바꾸려면 BP 서브클래스를 만들어 주입 액션에 등록.

## 여기서부터 읽어라
1. `Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 시스템의 중심. 소유·RPC·카메라·태그의 설계 근거가 헤더 주석에 집약
2. `Source/WxDialogue/Public/WxDialogueTableRow.h` — 대화 표현(노드=대사 한 줄=행)을 먼저 잡아야 세션 순회가 읽힌다
3. `Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — `EnterRow`/`Advance` 순회, `State.Dialogue` 태그 부착/해제, 카메라 구도 산출의 실제 흐름
4. `Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` — 관찰(Wait)/출력(Play) 두 노드의 역할 분리와 완주 판정 규약

## 관련
- 소비자: [[WxUI]](태그·델리게이트 구독으로 위젯 연결), [[WxQuest]](ST 노드로 대화 관찰·출력)
- 상위 계약: [[WxCore]](`IWxInteractable`·`State.Dialogue` 태그)

---
*문서 기준 커밋 `c549ea2` · 생성일 2026-07-31 · 소스 11파일 — `/readme-writer`로 갱신*
