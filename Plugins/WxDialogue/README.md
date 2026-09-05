# WxDialogue — 대화 시스템

> NPC·물체에 말을 걸어 데이터테이블에 적힌 대사를 한 줄씩 넘기며 재생한다. 대화 동안 전용 카메라와 화자 포즈를 세우고, 진행 중인 대사를 관찰 가능한 상태로 노출한다.

## 책임
**담당**
- 대화 정의: 대사 한 줄 = 테이블 행, 대화 1편 = 테이블 1개 (`FWxDialogueTableRow`, `NextRow` 로 노드 연결)
- 세션 진행: 현재 노드 추적·대사 넘기기·종료를 PlayerController 측에서 소유 (`UWxDialogueSessionComponent`)
- 대화 연출: 전용 카메라 구도(FOV·거리·오프축)·화자 포즈 몽타주 비동기 스트리밍
- 진입점 노출: 상호작용으로 여는 호스트 액터(`AWxDialogueActor`)와 데이터로 여는 StateTree 태스크
- 세션 개폐 신호: 폰 ASC에 `State.Dialogue` 루즈 태그를 올렸다 내림

**경계 (비담당)**
- 대화 창 UI 렌더링 — `State.Dialogue` 태그를 보는 [[WxUI]]가 여닫음. 이 모듈은 창 시작·종료 델리게이트를 두지 않고 대사 변경 델리게이트(`OnLineChanged`)만 발행
- 대사의 의미 판정(퀘스트 수주 등) — 진행 중 행(`GetCurrentRowHandle`)을 관찰하는 소비자([[WxQuest]] 등)의 몫. 이 모듈은 뜻을 해석하거나 기록하지 않음
- 상호작용 계약 인터페이스 `IWxInteractable`·`State.Dialogue` 태그 정의 — [[WxCore]] 제공

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 데이터의 단위(대사·화자·포즈·NextRow). 모든 흐름의 원천 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `UWxDialogueSessionComponent` | 세션 진행의 심장 — PC에 붙어 노드·카메라·포즈·태그를 모두 주관 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `AWxDialogueActor` | 말 걸 수 있는 대상의 추상 호스트, `IWxInteractable` 구현 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 대상이 어느 노드에서 시작할지만 보유(진행은 세션이 소유) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 액터 없이 데이터로 대화를 여는 StateTree 진입점(퀘스트용) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- 말 거는 새 대상 추가: `AWxDialogueActor`를 상속해 몸통을 세우고(루트는 파생이 만듦) `GetPoseMesh()`를 채운다. `UWxDialogueComponent`를 아무 액터에 붙여도 상호작용은 생기지 않는다 — 계약은 호스트 액터가 든다
- 데이터 주도: 대화 = `FWxDialogueTableRow` 타입 DataTable. `StartRow` 핸들로 시작 노드를 가리키고 `NextRow=None`이 종료. `TargetPose`는 소프트 참조라 세션이 대사를 넘길 때 비동기 스트리밍
- 진행 소유 모델: 세션은 소유 클라의 표시 전용 로컬 상태로 서버 검증이 없다(v1 싱글/리슨 호스트 전제). 대상은 비소유 액터라 UI로 가는 전달을 PC측 복제 컴포넌트가 Client RPC로 넘긴다
- 세션 전제: 폰 ASC가 없으면 `State.Dialogue`를 올릴 곳이 없어 세션을 열지 않음. 카메라는 로컬 컨트롤러에서만 동작(나레이션이면 카메라 미개입)
- 종료 대기: 폴링 대신 세션의 일회성 `OnDialogueEnded`에 붙는다

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 단위를 먼저 잡아야 전체가 보인다
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 진행·카메라·포즈·태그의 모든 결정과 그 이유가 헤더 주석에 모여 있다
3. `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — 노드 순회·RPC·스트리밍의 실제 구현
4. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` — 상호작용에서 세션으로 이어지는 진입 경로

## 관련
- 상위: 호스트 액터를 상호작용으로 부르는 [[WxCore]]의 `IWxInteractable`, 세션을 PC에 주입하는 Experience, 진행 대사를 관찰해 의미를 판정하는 [[WxQuest]], `State.Dialogue` 태그로 창을 여닫는 [[WxUI]]

---
*문서 기준 커밋 `f826b21` · 생성일 2026-09-05 · 소스 11파일 — `/readme-writer`로 갱신*
