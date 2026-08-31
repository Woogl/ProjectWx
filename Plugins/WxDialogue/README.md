# WxDialogue — 대화 시스템

> NPC·물체에 말을 걸어 DataTable 한 편을 대사 한 줄씩 진행시키는 런타임 플러그인. 대화 카메라 연출과 대상 포즈까지 세션이 함께 든다.

## 책임
**담당**
- 대화 데이터 정의: 대화 1편 = DataTable 1개, 노드 1개 = 대사 1행(`FWxDialogueTableRow`), `NextRow`로 이어지고 `None`이면 종료.
- 말 걸기 호스트와 상호작용 계약: 액터가 `IWxInteractable`을 받아 대화 컴포넌트로 넘긴다.
- 세션 진행: PlayerController 측에서 현재 노드·대사를 소유하고 `Advance`로 넘기며, 대사 변경을 델리게이트로 발행한다.
- 연출: 대화 전용 카메라 구도 설정·전환, 대사별 대상 포즈(몽타주) 비동기 스트리밍·재생.
- StateTree 연동: 액터의 대화 정의 대신 트리가 대사를 골라 여는 `Play Dialogue` 태스크(독백·무전·처치 후 대사).

**경계 (비담당)**
- 상호작용 인터페이스(`IWxInteractable`)·`State.Dialogue` 태그·ASC 정의는 [[WxCore]]가 든다.
- 대화 창(위젯) 개폐·표시는 `State.Dialogue` 태그를 보는 UI 쪽 몫. 세션은 대사만 발행한다.
- 대화의 의미 해석(퀘스트 수주 등)은 현재 행을 관찰하는 소비자 몫. 세션은 뜻을 해석하지도 기록을 남기지도 않는다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대사 한 줄 = 행 하나. 화자·대사·포즈·`NextRow`. | `Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `AWxDialogueActor` | 말 걸 수 있는 대상의 추상 호스트. 상호작용 계약을 컴포넌트로 넘긴다. | `Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 대상이 든 대화 정의(시작 행·화자 이름). 진행은 소유하지 않는다. | `Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | PC에 붙어 세션(현재 노드·라인)·카메라·포즈를 진행·소유한다. | `Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 트리가 대사를 골라 여는 StateTree 태스크. 종료까지 Running. | `Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- 새 대화: DataTable을 `FWxDialogueTableRow` 행 타입으로 만들고, 각 행에 화자·대사를 채운다. 모든 행은 `NextRow`를 정하며 종료는 `NextRow=None`(빈 값은 잘못된 행으로 경고 후 접힘).
- 새 말 대상: `AWxDialogueActor`를 상속해 몸통(캡슐+스켈레탈, 메시 등)을 세우고, 포즈를 얹을 메시는 `GetPoseMesh()`로 노출한다. `UWxDialogueComponent`의 `StartRow`가 비어 있으면 대화가 열리지 않는다.
- 트리 주도 대사: `Play Dialogue` 태스크에 `StartRow`를 지정하면 대상 액터의 대화 정의 없이도 재생된다. 대상이 없어 카메라는 플레이어에 머문다(나레이션).
- 전제: 세션 개폐는 폰 ASC의 `State.Dialogue` 태그로 알려지므로, 태그를 올릴 ASC가 없으면 세션을 열지 않는다. v1은 싱글/리슨 호스트(소유 클라=권위) 전제이며, 세션 상태는 표시 전용 로컬 상태로 서버 검증이 없다.

## 여기서부터 읽어라
1. `Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 세션 소유 근거·복제·카메라·포즈 정책이 헤더 주석에 모두 담겨 있다. 이 모듈의 중심.
2. `Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 모델. 대화가 어떻게 이어지고 끝나는지의 규약.
3. `Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` — 액터 밖에서 대화를 여는 두 번째 진입점.

## 관련
- 상위: 대화를 여는 상호작용 흐름·태스크 소비자. `State.Dialogue` 태그를 보는 UI가 대화 창을 띄운다.

---
*문서 기준 커밋 `b3f982b` · 생성일 2026-08-31 · 소스 11파일 — `/readme-writer`로 갱신*
