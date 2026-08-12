# WxDialogue — 대화 시스템

> 데이터 테이블로 정의한 대사를 NPC 상호작용이나 퀘스트 StateTree에서 재생하는 대화 시스템. 대상 컴포넌트·플레이어 세션·StateTree 노드로 나뉜다.

## 책임
**담당**
- 대화 데이터 정의(`FWxDialogueTableRow`): 대화 1편 = 테이블 1개, 노드 1개 = 행 1개, `NextRow`로 진행/종료.
- 대화 상대(`UWxDialogueComponent`): 액터에 붙여 상호작용 대상이 되고(`IWxInteractable`), 시작 행 보유와 상호작용 활성 토글.
- 플레이어 대화 세션(`UWxDialogueSessionComponent`): PC에 주입되어 세션 진행(현재 노드/라인), 대화 카메라 연출, 대상 포즈 스트리밍, State.Dialogue 태그 발행.
- 소비 도메인용 StateTree 태스크: 대화 관찰(대기)·출력·NPC 상호작용 토글.

**경계 (비담당)**
- 대화의 의미 해석(수주·납품 판정): 관찰만 노출하고 판정은 소비자(퀘스트 데이터)의 몫.
- 대화 창 UI: 대사 변경을 델리게이트로 발행할 뿐, 창을 여닫는 것은 State.Dialogue 태그를 보는 UI 쪽.
- 세션 서버 검증: v1은 싱글/리슨 호스트 전제라 표시 전용 로컬 상태로 진행.

## 의존성
- **주요 의존**: `WxCore`(`IWxInteractable`, `WxGameplayTags::State_Dialogue`), GameplayAbilities(ASC 태그), StateTree(태스크 노드), ModularGameplay(컨트롤러 주입), UniversalObjectLocator(NPC 지목).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 노드 1행(Speaker·Line·TargetPose·NextRow) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `UWxDialogueComponent` | 대화 상대 컴포넌트, `IWxInteractable` 구현·시작 행 보유 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | PC 주입 세션, 진행·카메라·포즈·태그 소유 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_WaitDialogueCompleted` | 지정 대화의 완주를 관찰(게이트) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_WaitDialogueCompleted.h` |
| `FWxStateTreeTask_PlayDialogue` | 트리가 대사를 열어 연출 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |
| `FWxStateTreeTask_EnableNpcInteraction` | 지정 NPC들의 상호작용 토글 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_EnableNpcInteraction.h` |

## 확장 포인트 / 규약
- **새 대화 추가**: `FWxDialogueTableRow` 기반 DataTable을 만들고 행들을 `NextRow`로 잇는다. 종료는 `NextRow=None`, `Line`은 모든 행이 채워야 한다(빈 행은 경고와 함께 접힘). 대상 컴포넌트의 `StartRow`나 태스크의 행 핸들로 지목.
- **NPC를 대화 상대로**: 액터에 `UWxDialogueComponent`를 붙이고 `SetAreaMesh`로 영역 메시를 지정, `StartRow`·`SpeakerName` 설정. 전용 액터 클래스 불필요. 잠긴 채 시작하려면 영역 메시 콜리전을 미리 꺼 둔다.
- **카메라 연출 조정**: 세션의 카메라 파라미터는 주입 기본값이라 값을 바꾸려면 `UWxDialogueSessionComponent`의 BP 서브클래스를 만들어 주입 액션에 등록.
- **퀘스트/StateTree에서 사용**: 소비 도메인은 대화 모듈을 참조하지 않고 에셋에서 세 태스크를 골라 쓴다(대화 노드를 이 모듈이 함께 제공 — 보상 노드를 [[WxInventory]]가, 인디케이터 노드를 [[WxUI]]가 소유하는 것과 같은 모양). NPC 지목은 `FUniversalObjectLocator` 배열. 새 태스크는 태스크당 헤더/소스 한 쌍(`Public|Private/WxStateTreeTask_<이름>.{h,cpp}`)으로 추가한다.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 모델. 대화의 최소 단위와 진행/종료 규약.
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 시스템의 심장. 진입점(StartDialogue/StartDialogueRow/Advance)·세션 상태·카메라·포즈의 설계 근거가 헤더 주석에 모여 있다.
3. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` — 상호작용에서 세션으로 넘어가는 위임 경로.
4. `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_*.h` — 퀘스트 등 소비 도메인이 대화를 관찰·출력·토글하는 세 태스크(태스크당 파일 하나).

## 관련
- 상위: [[WxCore]](../WxCore/README.md)

---
*문서 기준 커밋 `1ec70f2` · 생성일 2026-08-10 · 소스 9파일 — `/readme-writer`로 갱신*
