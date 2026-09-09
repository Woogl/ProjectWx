# WxDialogue — 대화 시스템

> 말을 걸 수 있는 대상과 나누는 대사의 흐름을 담당한다. DataTable 로 짠 대사 노드를 이어 재생하고, 대화 동안 카메라·NPC 포즈·상태 태그를 연출·발행한다.

## 책임
**담당**
- 대화 데이터 모델: 한 행 = 한 대사 노드, `NextRow` 로 이어지는 편(編) 단위 진행 (`FWxDialogueTableRow`)
- 세션 진행: 현재 노드 추적, 대사 넘기기(`Advance`), 종료 판정 — 플레이어 컨트롤러 측이 소유
- 대화 연출: 전용 대화 카메라 세팅/블렌드, 대사별 NPC 포즈 비동기 스트리밍·적용
- 상호작용 대상 호스팅: 말 걸 수 있는 액터의 베이스와 대화 시작 진입점
- StateTree 태스크로 퀘스트 등 외부 흐름이 특정 대사를 열도록 노출

**경계 (비담당)**
- 상호작용 계약(`IWxInteractable`)·`State.Dialogue` 등 공용 태그 정의는 [[WxCore]] 소유. 여기선 구현·참조만 한다.
- 대화 창 UI(위젯·뷰모델)는 그리지 않는다 — 대사 변경은 델리게이트로, 세션 개폐는 폰 ASC 의 `State.Dialogue` 태그로 발행하고 [[WxUI]] 가 관찰해 여닫는다.
- 대사의 의미 해석(퀘스트 수주 등)은 하지 않는다 — 소비자([[WxQuest]] 등)가 현재 행을 관찰해 판정한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 데이터의 단위. 대사·화자·포즈·`NextRow` 를 담은 DataTable 행 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `UWxDialogueSessionComponent` | 진행의 심장부. PC 에 붙어 세션·카메라·포즈·태그를 모두 든다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `UWxDialogueComponent` | 대상 액터 측. 시작 행만 보유하고 세션 진행은 넘긴다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `AWxDialogueActor` | 말 걸 수 있는 대상의 추상 호스트. `IWxInteractable` 을 세션 컴포넌트로 잇는다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` |
| `FWxStateTreeTask_PlayDialogue` | 대상 없이 트리가 대사를 여는 진입점. 종료까지 Running | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- 새 대화편: `WxDialogueTableRow` 타입 DataTable 을 만들고 행을 이어 붙인다. 모든 행은 `NextRow` 를 채워야 하며 종료는 `NextRow = None` 으로 표시한다(빈 값은 잘못된 행으로 경고).
- 새 대화 대상: `AWxDialogueActor` 를 상속해 몸통(메시/캡슐)을 세우고, 포즈를 얹을 메시가 있으면 `GetPoseMesh()` 를 오버라이드한다. 베이스는 루트를 만들지 않는다.
- 흐름 주도 대사(퀘스트 등): `FWxStateTreeTask_PlayDialogue` 를 StateTree 에 놓고 `StartRow` 를 지정한다 — 대상 없이 세션을 열어 나레이션도 가능.
- 권위/복제: 세션은 소유 클라가 진행하는 표시 전용 로컬 상태(서버 검증 없음). 대상은 비소유 액터라 UI 전달용 Client RPC 를 PC 측 세션 컴포넌트가 소유한다. v1 은 싱글/리슨 호스트(소유 클라=권위 동일 머신) 전제.
- 세션의 전제는 폰 ASC — `State.Dialogue` 태그를 올릴 곳이 없으면 창을 띄울 방법도 없어 세션을 열지 않는다.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 세션·카메라·포즈·태그·복제 정책이 헤더 주석에 응축돼 있다. 모듈 전체 설계의 출발점.
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 모델. 대화가 어떻게 짜이고 어떻게 끝나는지.
3. `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — `EnterRow`·`ApplyCurrentPose`·`BeginDialogueCamera` 등 진행·연출 실구현.

## 관련
- 상위: 상호작용은 [[WxCore]] 의 `IWxInteractable` 로 걸리고, 대사 표시는 [[WxUI]] 가 `State.Dialogue` 태그·`OnLineChanged` 를 관찰해 담당한다. 흐름 주도 대사는 [[WxQuest]]/StateTree 에서 `Play Dialogue` 태스크로 연다.

---
*문서 기준 커밋 `e0e3ecc` · 생성일 2026-09-09 · 소스 11파일 — `/readme-writer`로 갱신*
