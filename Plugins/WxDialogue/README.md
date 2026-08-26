# WxDialogue — 대화 시스템

> NPC·오브젝트에게 말을 걸어 데이터테이블에 정의된 대사를 순서대로 재생하고, 대화 동안 전용 카메라 구도와 대상 포즈를 연출한다. 대사의 의미(퀘스트 수주 등)는 해석하지 않고 "지금 어느 대사인가"만 노출한다.

## 책임
**담당**
- 대화 데이터 모델: 한 편의 대화 = DataTable 하나, 한 행 = 대사 한 줄(`FWxDialogueTableRow`). `NextRow` 링크로 노드를 이어가고 `None`이면 종료.
- 말 걸기 대상의 호스트·컴포넌트 조립: 상호작용 계약을 든 `AWxDialogueActor`, 대화 정의를 든 `UWxDialogueComponent`.
- 플레이어별 세션 진행: 현재 노드·라인 상태 소유, 대사 넘기기(`Advance`), 대화 카메라 세우기/되돌리기, 대상 포즈 비동기 스트리밍·재생(`UWxDialogueSessionComponent`).
- 대사 변경·대화 종료 발행: `OnLineChanged` 델리게이트로 뷰모델에 대사 통지, `OnDialogueEnded` 일회성 신호로 종료 대기자에게 통보.
- StateTree 진입점: 퀘스트 등 액터가 아닌 쪽이 대사를 직접 여는 `FWxStateTreeTask_PlayDialogue` 태스크.

**경계 (비담당)**
- 상호작용 감지·사거리·프롬프트 표시: 몸통 형상과 상호작용 스캔은 대상의 파생 액터/다른 시스템 몫. 본 모듈은 `IWxInteractable`([[WxCore]]) 계약만 구현한다.
- 대화 창 UI 여닫기·렌더링: `State.Dialogue` 태그와 `OnLineChanged`를 관찰하는 UI 매니저([[WxUI]])의 몫. 시작·종료 델리게이트를 두지 않는 이유.
- 대사의 의미 판정(퀘스트 수주 등): 현재 행을 관찰하는 소비자([[WxQuest]] 등)가 판정한다.
- 세션 주입: `UWxDialogueSessionComponent`를 PlayerController에 붙이는 것은 Experience 시스템([[WxGame]])의 몫.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 데이터 행(Speaker·Line·TargetPose·NextRow) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `AWxDialogueActor` | 말 걸기 대상의 추상 호스트, `IWxInteractable` 구현체 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 대상의 대화 정의(시작 행·표시 이름) 보유 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | 플레이어 측 세션 진행·카메라·포즈·발행의 중추 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 액터 없이 행을 직접 여는 ST 태스크(독백·무전 등) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- **새 대화 대상 추가**: `AWxDialogueActor`를 상속(루트 없음 — 파생이 몸통을 세운다)하고 `UWxDialogueComponent`를 붙여 `StartRow`·`SpeakerName`을 인스턴스별로 지정한다. `UWxDialogueComponent`만 단독으로 아무 액터에 붙여선 말을 걸 수 없다(계약은 액터 전용).
- **새 대화 콘텐츠**: `RowType = WxDialogueTableRow`인 DataTable을 만들고 행마다 `NextRow`로 흐름을 연결, 종료 행은 `NextRow = None`으로 표시한다. `Line`이 비면 잘못된 행으로 보고 경고 후 대화를 접는다.
- **포즈 연출**: 행의 `TargetPose`는 소프트 참조(`TSoftObjectPtr`) — 세션이 대사를 넘길 때 비동기 스트리밍한다. 지목이 없으면 직전 포즈 유지, 종료해도 되돌리지 않는다.
- **진입 경로 두 가지**: 상호작용 → `UWxDialogueComponent::StartDialogueWith` → 세션 `StartDialogue`; 액터 없는 대사 → 세션 `StartDialogueRow`(또는 ST 태스크).
- **리플리케이션 모델**: 세션 컴포넌트는 복제 컴포넌트지만 세션 상태(현재 노드·라인)는 소유 클라의 표시 전용 로컬 상태이고 서버 검증이 없다. 서버 권위 진입점이 `Client RPC`(`ClientStartDialogue`)로 소유 클라에 세션을 시드한다. v1은 싱글/리슨 호스트(소유 클라=권위 동일 머신) 전제 — 관찰자는 로컬 상태를 직접 읽는다.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 모듈의 중추. 클래스 doc-comment가 리플리케이션·카메라·포즈·발행 정책을 모두 설명한다.
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 모델. 대화가 어떻게 노드로 이어지는지 여기서 잡힌다.
3. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` + `WxDialogueComponent.h` — 상호작용 → 대화 진입의 액터/컴포넌트 분업.

## 관련
- 상위: 상호작용 계약은 [[WxCore]]의 `IWxInteractable`. 대화 창 UI는 [[WxUI]], 대사 관찰 소비자는 [[WxQuest]]. 세션 컴포넌트 주입과 Experience는 [[WxGame]]. ST 태스크는 [[WxAI]]/[[WxQuest]]의 StateTree에서 사용.

---
*문서 기준 커밋 `807a9da` · 생성일 2026-08-22 · 소스 11파일 — `/readme-writer`로 갱신*
