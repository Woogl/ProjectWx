# WxDialogue — 대화 시스템

> 배치된 대상에 말을 걸거나 퀘스트가 직접 여는 데이터 주도 대화. 대상은 대화 정의만 들고, 진행·카메라·포즈·UI 발행은 플레이어 컨트롤러의 세션이 소유한다.

## 책임
**담당**
- DataTable(`FWxDialogueTableRow`) 한 편을 노드로 순회하며 화자·대사를 발행하고 `NextRow`로 진행/종료
- 상호작용으로 말을 거는 대화 대상(`AWxDialogueActor`) 호스팅과 대화 정의 컴포넌트(`UWxDialogueComponent`)
- 플레이어 측 세션 소유: 현재 노드/라인 로컬 상태, 대화 전용 카메라 구도·블렌드, 대사별 NPC 포즈 소프트 스트리밍, `State.Dialogue` 태그 발행
- StateTree 태스크(`FWxStateTreeTask_PlayDialogue`)로 퀘스트가 대상 없이(독백·무전) 대사를 여는 진입점

**경계 (비담당)**
- 상호작용 감지·사거리·계약 정의(`IWxInteractable`), `WxGameplayTags` — [[WxCore]]
- 대화 창을 여닫는 UI: `State.Dialogue` 태그와 `OnLineChanged` 델리게이트를 뷰모델/UI 매니저가 관찰 — [[WxUI]]
- 대화의 의미 해석(퀘스트 수주 등): 소비자가 현재 행 신원을 관찰로 판정 — [[WxQuest]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 노드 1행 = 대사 1줄(화자·대사·포즈·NextRow) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `AWxDialogueActor` | 말 거는 대상의 추상 호스트, 상호작용 계약을 컴포넌트로 위임 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 대상의 대화 정의(시작 행·화자명), 세션 진입 트리거 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | PC 주입 세션: 진행·카메라·포즈·라인 발행·종료 신호 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 퀘스트 ST에서 행을 직접 열고 종료까지 Running | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- 새 대화 편: DataTable 에셋을 `FWxDialogueTableRow` 로우 타입으로 만들고, 행끼리 `NextRow` 로 잇는다(종료는 `NextRow=None`). 모든 행은 `Line`을 채워야 하며 비면 경고 후 접힌다.
- 새 대화 대상: `AWxDialogueActor` 를 상속해 몸통을 세우고 `UWxDialogueComponent` 에 시작 행을 지정한다. 컴포넌트만 붙여선 말을 걸 수 없다(계약은 액터 전용).
- 퀘스트 대사: `FWxStateTreeTask_PlayDialogue` 의 `StartRow` 를 지정. 대상 없이 열려 카메라는 플레이어에 머문다.
- NPC 포즈: 각 행의 `TargetPose`(소프트) 지정 시 세션이 대사 넘길 때 비동기 스트리밍. 비우면 직전 포즈 유지, 대화 종료 후에도 되돌리지 않는다.
- 카메라 구도는 세션의 `EditDefaultsOnly` 값(FOV·오프축각·거리·높이·블렌드)으로 조정 — 주입 컴포넌트라 기본값이 곧 실제값.
- v1 전제: 싱글/리슨 호스트(소유 클라=권위 동일 머신), 0번 컨트롤러. 세션은 표시 전용 로컬 상태라 서버 검증 없음.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 진행·카메라·포즈·발행의 소유 지점이자 설계 근거가 모인 곳
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 대화 데이터 모델(노드·종료 규약)
3. `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — 상호작용/ST 두 진입에서 클라 세션이 열리고 닫히는 실제 흐름

## 관련
- 상위: [[WxCore]]
- 소비: [[WxQuest]], [[WxUI]]

---
*문서 기준 커밋 `718b827` · 생성일 2026-08-26 · 소스 11파일 — `/readme-writer`로 갱신*
