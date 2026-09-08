# WxDialogue — 대화 시스템

> 데이터 테이블로 정의한 대사를 상호작용으로 열고, 소유 클라 측 세션이 한 줄씩 진행하며 카메라·NPC 포즈·표시 신호를 연출한다.

## 책임
**담당**
- 대화 데이터 모델: 행 하나 = 대사 한 줄, `NextRow` 로 이어지는 테이블 1편(`FWxDialogueTableRow`)
- 말을 걸 수 있는 대상의 호스트 액터와 시작점 보유 컴포넌트(`AWxDialogueActor`, `UWxDialogueComponent`)
- 세션 진행: 현재 행 추적·대사 넘기기·종료, PlayerController 측에서 소유(`UWxDialogueSessionComponent`)
- 대화 연출: 전용 카메라 구도·뷰 타겟 전환, NPC 포즈 몽타주 비동기 스트리밍·재생
- 세션 개폐 신호: 폰 ASC에 `State.Dialogue` 루즈 태그를 올렸다 내림
- 퀘스트 등 비액터 소비자를 위한 직접 행 재생 진입점과 StateTree 태스크(`FWxStateTreeTask_PlayDialogue`)

**경계 (비담당)**
- 대화 창 UI 렌더링 — `State.Dialogue` 태그를 보는 [[WxUI]]가 여닫음. 이 모듈은 창 시작·종료 델리게이트를 두지 않고 대사 변경 델리게이트(`OnLineChanged`)만 발행
- 대사의 의미 판정(퀘스트 수주 등) — 진행 중인 행을 관찰하는 소비자([[WxQuest]])의 몫
- 상호작용 계약 인터페이스 `IWxInteractable`·`State.Dialogue` 태그 정의 — [[WxCore]] 제공

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 노드 한 줄(화자·대사·포즈·다음 행) | `Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `AWxDialogueActor` | `IWxInteractable`을 구현한 대화 대상 추상 베이스 | `Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 대상에 붙어 시작 행만 보유, 세션으로 위임 | `Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | 세션 진행·카메라·포즈·태그의 실체, PC가 소유 | `Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 대상 없이 트리가 대사를 소유하는 ST 태스크 | `Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- 새 대화 대상: `AWxDialogueActor`를 상속하고 몸통(캡슐+스켈레탈 등)을 직접 세운다. 베이스는 루트를 만들지 않으며, 포즈를 얹을 메시는 `GetPoseMesh()`로 노출한다.
- 대화 편집은 데이터 주도: `FWxDialogueTableRow` 데이터 테이블 한 개가 대화 1편. 종료는 `NextRow=None`, 포즈는 `TSoftObjectPtr`라 세션이 넘길 때 비동기 스트리밍한다.
- 두 진입: 대상 액터가 소유한 정의는 `StartDialogue(UWxDialogueComponent*)`, 대사를 트리 등이 소유하면 `StartDialogueRow(Handle, Target)`(Target 없으면 나레이션, 카메라 미개입).
- 세션 전제: 폰 ASC가 없으면 `State.Dialogue`를 올릴 곳이 없어 세션을 열지 않는다. 카메라는 로컬 컨트롤러에서만 동작.
- StateTree 태스크는 폴링하지 않고 세션의 일회성 `OnDialogueEnded`에 붙어 완료를 통보받는다. 0번 컨트롤러 전제(v1 싱글/리슨 호스트).

## 여기서부터 읽어라
1. `Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 세션 소유 위치·복제 이유·카메라/포즈/태그 정책이 헤더 주석에 모여 있는 중심
2. `Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — 서버 진입 → Client RPC → 행 진입 → 대사 발행 → 종료의 실제 흐름
3. `Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 모델이 곧 대화 진행 규약

## 관련
- 상위: 호스트 액터를 상호작용으로 부르는 [[WxCore]]의 `IWxInteractable`, 세션을 기본 서브오브젝트로 드는 [[WxGame]]의 PlayerController, 진행 대사를 관찰해 의미를 판정하는 [[WxQuest]], `State.Dialogue` 태그로 창을 여닫는 [[WxUI]]

---
*문서 기준 커밋 `ba86cff` · 생성일 2026-09-08 · 소스 11파일 — `/readme-writer`로 갱신*
