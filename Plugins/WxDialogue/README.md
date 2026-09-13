# WxDialogue — 대화 시스템

> 말을 걸 수 있는 대상과 나눈 대화 한 편을 데이터 테이블로 정의하고, 대사를 한 줄씩 넘기며 카메라·포즈 연출과 함께 재생한다. 대사의 신원만 노출할 뿐 의미(퀘스트 수주 등)는 해석하지 않는다.

## 책임
**담당**
- 대화 데이터 스키마(행=대사, `NextRow` 체인으로 이어지는 노드 그래프)
- 대화 세션 진행 — 현재 행 추적, 대사 넘기기, 종료 판정
- 대화 연출 — 전용 카메라 구도/블렌드, 대사별 NPC 포즈(소프트 참조 비동기 스트리밍)
- "지금 어느 대사인가"의 신원 노출(현재 행 핸들·화자·대사, 대사 변경 델리게이트)

**경계 (비담당)**
- 상호작용 트리거 계약: `IWxInteractable`은 [[WxCore]] 정의, 호스트 액터가 구현만 한다
- 대화 창 UI: 세션은 폰 ASC의 `State.Dialogue` 태그와 대사 델리게이트만 발행하고, 여닫는 것은 [[WxUI]]의 몫
- 대화가 뜻하는 바(퀘스트 진행 등): 소비자([[WxQuest]] 등)가 현재 행을 관찰로 판정
- 어떤 대사를 낼지 트리 주도로 고르기: [[WxQuest]]/[[WxAI]]의 StateTree가 Play Dialogue 태스크로 지정

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 데이터 스키마 — 화자·대사·포즈·`NextRow`. 대화 1편 = 테이블 1개 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `AWxDialogueActor` | 말 걸 대상의 추상 호스트. `IWxInteractable` 구현부, 파생이 몸통을 세운다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 호스트에 붙어 시작 행만 보유. 상호작용을 세션으로 넘기는 다리 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | 진행 주체. PC에 붙어 세션·카메라·포즈·태그를 소유 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 트리 주도 진입점 — 대상 없이 행을 직접 지정해 세션을 열고 종료까지 대기 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- **새 대화 대상**: `AWxDialogueActor`를 파생해 몸통(캡슐+스켈레탈, 혹은 메시)을 세우고 필요 시 `GetPoseMesh()`를 재정의한다. 기반 클래스는 루트를 만들지 않는다.
- **대화 정의**: `FWxDialogueTableRow` 로우 타입의 DataTable을 만들고 `UWxDialogueComponent::StartRow`에 시작 행을 지정. 종료는 `NextRow = None`.
- **두 진입 경로**: 상호작용 대상이 자기 대사를 낼 땐 `StartDialogue(UWxDialogueComponent*)`, 트리가 대사를 고를 땐 `StartDialogueRow(Handle, Target)`(Target 비우면 나레이션, 카메라는 플레이어에 잔류).
- **종료 관찰**: 트리 등 종료 대기자는 대화를 연 직후 `OnDialogueEnded`(1회 발화 후 self-clear)를 붙인다.
- **전제**: v1 싱글/리슨 호스트(소유 클라=권위 동일 머신). 폰에 ASC가 없으면 `State.Dialogue`를 올릴 곳이 없어 세션을 열지 않는다.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 세션 소유·복제·카메라·포즈·태그 정책이 헤더 주석에 모여 있다. 이 모듈의 중심.
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 모델. 대화가 어떻게 이어지고 끝나는지의 원천.
3. `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — 행 진입/스트리밍/카메라 계산의 실제 구현.

## 관련
- 상위: [[WxQuest]]·[[WxAI]] StateTree가 Play Dialogue 태스크로 대사를 낸다. 세션 상태는 [[WxUI]](대화 창)와 관찰 소비자가 읽는다.
- 하위: [[WxCore]]의 `IWxInteractable`·`WxGameplayTags`(`State.Dialogue`)에 의존.

---
*문서 기준 커밋 `eda01fd` · 생성일 2026-09-13 · 소스 11파일 — `/readme-writer`로 갱신*
