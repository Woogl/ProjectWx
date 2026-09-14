# WxDialogue — 대화 시스템

> 말을 걸 수 있는 대상과 플레이어 사이의 대사 세션을 데이터 테이블 기반으로 진행한다. 대사를 넘기며 카메라·NPC 포즈를 함께 연출하고, 대화의 의미 판정은 하지 않는다.

## 책임
**담당**
- 대화 대상 호스트 액터와 그 대화 정의(어느 행에서 시작할지) 보유
- 플레이어 컨트롤러 측이 소유하는 대화 세션 진행(현재 노드·라인, 넘기기, 종료)
- 대사 데이터 스키마(화자·대사·포즈·다음 행)를 담는 데이터 테이블 행 타입
- 세션 동안의 전용 대화 카메라 연출과 대상 포즈(몽타주) 비동기 스트리밍·적용
- 세션 개폐를 폰 ASC 의 `State.Dialogue` 태그로 알림, 대사 변경을 델리게이트로 발행
- 퀘스트 등 StateTree 흐름에서 대사를 직접 열고 종료를 기다리는 태스크 노드

**경계 (비담당)**
- 대화 창 UI 개폐·표시: [[WxUI]] (`State.Dialogue` 태그와 `OnLineChanged` 델리게이트를 받아 처리)
- 상호작용 진입(`IWxInteractable`)·`State.Dialogue` 태그 정의: [[WxCore]]
- 대화가 끝난 뒤의 의미 판정(퀘스트 수주 등): 종료를 기다린 소비자([[WxQuest]] 등)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `AWxDialogueActor` | 말 걸 수 있는 대상의 추상 호스트. `IWxInteractable` 을 받아 `UWxDialogueComponent` 로 넘김 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 대상에 붙어 시작 행·화자명만 보유. 서버 권위에서 세션을 연다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | PC 기본 서브오브젝트. 세션 진행·카메라·포즈·태그·델리게이트를 소유 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxDialogueTableRow` | 대화 노드 1개 = 대사 1줄. 화자·대사·포즈·NextRow | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `FWxStateTreeTask_PlayDialogue` | StateTree 태스크. 대상 없이 시작 행을 직접 열고 종료까지 Running | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- **새 대화**: 데이터 테이블(행 타입 `FWxDialogueTableRow`) 1개가 대화 1편. 행 하나가 노드 하나이며 `NextRow` 로 잇고 `NextRow=None` 이 종료. `Line` 은 모든 행이 채워야 하고 비면 경고 후 대화를 접는다.
- **새 대상**: `AWxDialogueActor` 를 파생. 베이스는 루트를 만들지 않으므로 파생이 몸통(캡슐+스켈레탈 등)을 세우고, 포즈를 얹으려면 `GetPoseMesh()` 를 재정의한다.
- **두 갈래 진입**: 액터가 대사를 고르면 `StartDialogue(UWxDialogueComponent*)`, 트리·나레이션처럼 고르는 쪽이 액터가 아니면 `StartDialogueRow(Handle, Target)`(Target 을 비우면 카메라가 플레이어에 머문다).
- **세션 전제**: 폰 ASC 가 없으면(`State.Dialogue` 태그를 올릴 곳이 없으면) 세션을 열지 않는다. 빙의가 바뀌면 세션을 접는다.
- **카메라 구도**: `UWxDialogueSessionComponent` 의 `EditDefaultsOnly` FOV·오프축각·거리·높이·블렌드 값으로 조정.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 세션의 소유·권위 모델과 카메라·포즈·태그·델리게이트 책임이 헤더 주석에 모여 있다. 이 모듈의 중심.
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 대사 데이터가 어떻게 이어지고 끝나는지, 데이터 주도 설계의 뿌리.
3. `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` — 액터 밖(퀘스트 흐름)에서 대화를 여는 경로.

## 관련
- 상위: [[WxCore]]
- 협력: [[WxUI]] · [[WxQuest]]

---
*문서 기준 커밋 `dc08752` · 생성일 2026-09-14 · 소스 11파일 — `/readme-writer`로 갱신*
