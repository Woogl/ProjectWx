# WxDialogue — 대화 시스템

> NPC·물체에 말을 걸어 DataTable 로 짠 대사를 한 줄씩 넘기며 재생하고, 그 동안 전용 카메라와 대상 포즈로 연출을 세운다. 대화의 진행 상태만 노출할 뿐, 대사의 의미(퀘스트 수주 등)는 해석하지 않는다.

## 책임
**담당**
- 대화 정의(DataTable 행 그래프)를 따라 대사를 한 줄씩 진행·종료
- 소유 클라 측 세션 상태(현재 노드·화자·라인)와 대사 변경 델리게이트 발행
- 대화 연출: 두 사람 배치로 구도를 잡는 전용 카메라, 대사별 대상 포즈(몽타주) 비동기 스트리밍·재생
- 세션 개폐를 폰 ASC 의 `State.Dialogue` 태그로 표시
- 말 걸기 호스트 액터의 상호작용 계약 이행

**경계 (비담당)**
- 대사의 뜻 해석·기록: 하지 않음. 관찰자가 현재 행을 읽어 판정 (예: [[WxQuest]])
- 대화 창 UI 렌더링: `State.Dialogue` 태그와 `OnLineChanged` 를 보는 [[WxUI]] 몫
- 상호작용 프롬프트/판정 인터페이스(`IWxInteractable`)·`State.Dialogue` 태그 정의: [[WxCore]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 노드 1행 = 대사 한 줄. `NextRow` 로 그래프를 잇는 데이터 스키마 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `UWxDialogueSessionComponent` | PC 에 주입되는 세션 소유자. 진행·카메라·포즈·태그의 실제 로직이 전부 여기 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `UWxDialogueComponent` | 대상 액터가 드는 대화 정의(시작 행·화자명). 진행은 소유하지 않고 세션에 넘김 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `AWxDialogueActor` | 말 걸 수 있는 대상의 추상 호스트. `IWxInteractable` 를 대화 컴포넌트로 위임 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` |
| `FWxStateTreeTask_PlayDialogue` | 퀘스트 ST 진입점. 대상 액터 없이 지정 행을 열고 종료까지 대기 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- **새 대화 대상**: `AWxDialogueActor` 를 상속해 몸통(캡슐+스켈레탈, 또는 단순 메시)을 세우고, 포즈를 얹을 메시가 있으면 `GetPoseMesh()` 를 오버라이드한다(기본 null). 액터 자체는 `Abstract`, 루트 미생성.
- **데이터 주도**: 대화 1편 = `FWxDialogueTableRow` 로 만든 DataTable 1개. 각 행은 `NextRow` 를 반드시 채우고(종료는 `NextRow=None`), `Line` 이 비면 잘못된 행으로 보고 접는다. `TargetPose` 는 소프트 참조이며 세션이 대사를 넘길 때 비동기 스트리밍.
- **액터 없는 대사**: 화자를 코드/트리가 고를 땐 `StartDialogueRow(StartRow, Target)` — `Target` 을 비우면 나레이션(카메라는 플레이어에 머묾).
- **리플리케이션/권한**: v1 은 싱글/리슨 호스트(소유 클라=권위) 전제. 세션은 표시 전용 로컬 상태로 서버 검증이 없고, 진행은 소유 클라가 소유한다. 시작 시드는 `ClientStartDialogue` (Client RPC)로 내려가므로 세션 컴포넌트는 복제 컴포넌트다. 게임 상태를 바꾸는 대화가 생기면 서버측으로 옮긴다.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 헤더 주석이 왜 세션이 PC 에 붙는지·카메라와 포즈를 여기서 드는 이유·되돌림 정책까지 설계 근거를 담고 있다. 모듈의 중심.
2. `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — 실제 진행 루프(`EnterRow`/`Advance`/`EndDialogue`), 카메라 구도 계산, 포즈 스트리밍, `State.Dialogue` 태그 개폐.
3. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 대화 데이터 스키마. 콘텐츠가 실제로 만지는 계약.

## 관련
- 상위: 상호작용으로 대화를 여는 흐름은 [[WxCore]]의 `IWxInteractable`, 트리에서 대사를 여는 흐름은 [[WxAI]]/[[WxQuest]]의 StateTree. 진행 관찰·UI 는 [[WxUI]].

---
*문서 기준 커밋 `bb06a17` · 생성일 2026-08-30 · 소스 11파일 — `/readme-writer`로 갱신*
