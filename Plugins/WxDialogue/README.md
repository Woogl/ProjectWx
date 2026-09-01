# WxDialogue — 대화 시스템

> NPC·말 거는 물체와의 대사 진행을 담당한다. 대화 정의는 DataTable 행(노드)의 연쇄로 두고, 세션 진행은 상호작용한 플레이어의 PlayerController 컴포넌트가 소유한다.

## 책임
**담당**
- 대화 대상 호스트(`AWxDialogueActor`)와 상호작용 계약(`IWxInteractable`) 연결
- 대화 데이터 스키마(`FWxDialogueTableRow`): 화자·대사·포즈·다음 행의 연쇄
- 세션 진행(`UWxDialogueSessionComponent`): 현재 노드·라인 상태, 대사 넘기기, 대화 카메라 구도, 대상 포즈 스트리밍/재생
- StateTree 태스크(`FWxStateTreeTask_PlayDialogue`): 액터가 아니라 트리가 대사를 소유하는 독백·무전·처치 후 대사

**경계 (비담당)**
- 대화 창 여닫기·대사 뷰 표시 — 세션은 `State.Dialogue` 태그와 `OnLineChanged` 델리게이트만 발행하고, 관찰하는 쪽([[WxUI]])이 창을 띄운다
- 상호작용 발동·프롬프트 계약(`IWxInteractable`)과 `State.Dialogue` 태그 정의 — [[WxCore]]
- 대사의 의미(퀘스트 수주 등) 판정 — 소비자가 현재 행 관찰로 판정([[WxQuest]])

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxDialogueSessionComponent` | PC에 주입, 세션 진행·카메라·포즈를 소유하는 실행 중심 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `AWxDialogueActor` | 말 걸 수 있는 대상의 추상 호스트, 상호작용 계약 보유 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 호스트에 붙어 시작 행·화자명을 들고 세션을 연다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `FWxDialogueTableRow` | 대화 노드 1개 = 대사 1줄(화자·대사·포즈·다음 행) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `FWxStateTreeTask_PlayDialogue` | 트리가 대사를 소유해 세션에 여는 ST 태스크 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- **대화 1편 = DataTable 1개**, 행 하나가 노드 하나(대사 한 줄). 출력 후 `NextRow`로 이어가고 `NextRow=None`이면 종료. 모든 행은 `Line`을 채워야 하며 빈 행은 잘못된 조립으로 보고 경고와 함께 접힌다.
- 대상 포즈(`TargetPose`)는 소프트 참조 — 세션이 대사를 넘길 때 비동기 스트리밍한다. 대화가 끝나도 포즈는 마지막 자세로 남고, 다음 대사/대화가 갈아끼운다.
- 대화 진입 경로는 둘: 대상 액터의 `UWxDialogueComponent`(상호작용)와, 행을 직접 지정하는 `StartDialogueRow`(ST 태스크, 대상 없으면 나레이션).
- 세션은 뜻을 해석하지 않는다 — 현재 행 신원(`GetCurrentRowHandle`)과 대상만 노출하고, 소비자가 관찰로 판정한다. `HasActiveDialogue`로 가린 뒤 비교할 것.
- 폰 ASC는 세션의 전제다 — `State.Dialogue` 태그를 올릴 곳이 없으면 세션을 열지 않는다. v1은 싱글/리슨 호스트(소유 클라=권위 동일 머신) 전제.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 소유·복제·카메라·포즈 정책을 헤더 주석이 전부 설명한다. 모듈의 무게 중심.
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 대화 데이터의 최소 스키마이자 진행 규약(노드 연쇄).
3. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` — 상호작용에서 세션까지의 진입 흐름 시작점.

## 관련
- 상위: [[WxQuest]] (Play Dialogue 태스크 소비), [[WxUI]] (대화 창 표시)
- 기반: [[WxCore]] (`IWxInteractable`, `WxGameplayTags::State_Dialogue`)

---
*문서 기준 커밋 `ee3c177` · 생성일 2026-09-01 · 소스 11파일 — `/readme-writer`로 갱신*
