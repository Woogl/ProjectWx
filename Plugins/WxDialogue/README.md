# WxDialogue — 대화 시스템

> DataTable로 짠 대사 그래프를 재생하는 대화 시스템. 말 걸 대상은 컴포넌트로 표식하고, 세션 진행·카메라·NPC 포즈는 플레이어 컨트롤러 측이 소유한다.

## 책임
**담당**
- 대화 대상 표식 및 상호작용 진입점 (`UWxDialogueComponent`, `IWxInteractable` 구현)
- 대화 세션 진행 — 현재 노드·라인 상태, 대사 넘기기(`Advance`), 종료 판정 (`UWxDialogueSessionComponent`)
- 대화 데이터 스키마 — 노드=행, NextRow로 잇는 대사 그래프 (`FWxDialogueTableRow`)
- 대화 연출 — 전용 카메라 구도 전환, 대사별 NPC 포즈 몽타주 스트리밍/재생
- 대사 변경 발행(`OnLineChanged` 델리게이트)과 세션 개폐 신호(폰 ASC의 `State.Dialogue` 태그)
- StateTree에서 대사를 여는 태스크 (`FWxStateTreeTask_PlayDialogue`)

**경계 (비담당)**
- 대화 창 렌더링·여닫기 — [[WxUI]] (`State.Dialogue` 태그와 `OnLineChanged`를 관찰)
- 상호작용 감지·사거리 판정·상호작용 어빌리티 — [[WxCore]] (`IWxInteractable` 계약, 스캐너)
- 대사의 의미 해석(퀘스트 수주 등) — 소비자가 현재 행을 관찰로 판정 (예: [[WxQuest]])

## 의존성
- **주요 의존**: `WxCore` (`IWxInteractable`, `WxGameplayTags`) · GameplayAbilities(ASC 루즈 태그) · ModularGameplay(컨트롤러 컴포넌트 주입) · StateTree · UniversalObjectLocator
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxDialogueComponent` | 대화 대상 표식. 붙이면 어느 액터든 말 걸 수 있는 대상이 됨. `IWxInteractable` 구현, 상호작용 시 세션 시작 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | 플레이어 PC에 주입되는 세션 소유자. 진행·카메라·포즈·발행을 모두 담당 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxDialogueTableRow` | 대화 노드 1개(대사 한 줄). Speaker·Line·TargetPose·NextRow | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `FWxStateTreeTask_PlayDialogue` | ST에서 액터 없이 지정 대사를 열고 종료까지 대기하는 태스크 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- **새 대화 추가**: `FWxDialogueTableRow`를 RowType으로 갖는 DataTable 하나 = 대화 1편. 각 행이 노드, `NextRow`로 다음 행을 잇고 `NextRow=None`이면 종료. 모든 행의 `Line`은 채워야 하며 비면 경고 후 대화를 접는다.
- **대상에 대화 붙이기**: 액터에 `UWxDialogueComponent`를 붙이고 `StartRow`(시작 행 핸들)와 `AreaMesh`(상호작용 감지 형상)를 지정. 전용 C++ 액터 클래스는 불필요. `AreaMesh`의 쿼리 콜리전 on/off가 곧 상호작용 활성 상태(별도 플래그 없음).
- **액터 없는 대사(독백·무전·처치 후)**: StateTree에 `FWxStateTreeTask_PlayDialogue`를 두고 `StartRow`만 지정. 대상이 없어 카메라는 플레이어에 머문다. 대사는 트리가 소유하므로 같은 NPC라도 퀘스트 단계마다 다른 대사를 낼 수 있다.
- **세션 개폐 관찰**: 세션 동안 폰 ASC에 `State.Dialogue`(WxCore 선언) 루즈 태그가 붙는다. UI 등 소비자는 이 태그로 창을 여닫는다 — 시작·종료 델리게이트는 두지 않는다.
- **전제(v1)**: 싱글/리슨 호스트(소유 클라=권위 동일 머신). 세션은 표시 전용 로컬 상태로 서버 검증이 없다. StateTree 태스크는 0번 컨트롤러를 쓴다.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 시스템의 심장. 진입점(`StartDialogue`/`StartDialogueRow`), 진행(`Advance`), 카메라·포즈 정책, RPC 경계가 헤더 doc-comment에 모두 서술됨
2. `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp` — 상호작용→세션 시작으로 넘어가는 짧고 명확한 진입 흐름(`OnInteracted`)
3. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 스키마. 대화가 어떻게 그래프로 짜이는지의 출발점

## 관련
- 상위: [[WxGame]] · 계약 제공 [[WxCore]]
- 소비: [[WxUI]] (창 표시) · [[WxQuest]] (ST 태스크로 대사 재생, 현재 행 관찰)

---
*문서 기준 커밋 `e355c65` · 생성일 2026-08-19 · 소스 9파일 — `/readme-writer`로 갱신*
