# WxDialogue — 대화 시스템

> NPC·나레이션 대사를 데이터 테이블로 정의하고, 플레이어 컨트롤러에 붙는 세션이 한 줄씩 진행하며 대화 카메라·NPC 포즈를 연출한다. 대화의 "의미"는 해석하지 않고 진행 중인 행만 노출한다.

## 책임
**담당**
- 대화 정의 데이터 스키마(행 = 대사 한 줄, `NextRow` 체인, 화자·포즈)
- 말을 걸 수 있는 대상 만들기(`UWxDialogueComponent` — `IWxInteractable` 구현)
- 대화 세션 진행(현재 행 추적, 대사 넘기기, 시작/종료)과 소유 클라 전달(Client RPC)
- 대화 연출: 전용 대화 카메라 구도·블렌드, 대상 NPC 포즈 비동기 스트리밍/재생
- StateTree 태스크로 퀘스트가 특정 대사를 직접 트리거하는 진입점

**경계 (비담당)**
- 상호작용 감지·프롬프트 표시·계약 정의 → [[WxCore]] (`IWxInteractable`)
- 대화 창 UI(뷰모델·위젯) → [[WxUI]]. 본 모듈은 델리게이트/ASC 태그만 발행하고 창은 태그를 보는 쪽이 여닫는다
- 대화의 의미 판정(퀘스트 수주 등) → [[WxQuest]] 등 관찰 소비자가 현재 행을 읽어 판정

## 의존성
- **주요 의존**: `WxCore`(`IWxInteractable`, `WxGameplayTags`) · 엔진: StateTree(`StateTreeModule`), GameplayAbilities(ASC 루즈 태그), ModularGameplay(컨트롤러 컴포넌트 주입), UniversalObjectLocator
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (참조 Wx 플러그인은 `WxCore`뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대사 한 줄을 담는 DataTable 행(화자·대사·포즈·`NextRow`) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `UWxDialogueComponent` | 액터를 대화 대상으로 만드는 컴포넌트, 시작 노드 보유 + `IWxInteractable` | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | PC에 주입돼 세션 진행·카메라·포즈를 소유하는 본체 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 퀘스트 ST가 대사를 직접 여는 태스크(독백·무전·처치 후 대사) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |
| `FWxOnDialogueLineChanged` | 표시 대사 변경을 UI 뷰모델에 알리는 델리게이트 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |

## 확장 포인트 / 규약
- **새 대화**: DataTable(RowStruct = `FWxDialogueTableRow`) 하나가 대화 1편. 행이 `NextRow`로 이어지고 `NextRow=None`이면 종료. NPC엔 `UWxDialogueComponent`를 붙이고 `StartRow`를 지목한다 — 전용 C++ 액터 불필요.
- **말을 걸 수 있게 하기**: 상호작용 계약은 호스트 액터가 아니라 `UWxDialogueComponent`가 든다. `AreaMesh`의 쿼리 콜리전이 곧 상호작용 영역이자 on/off 상태이며, 퀘스트의 'Enable Interaction' 태스크가 `SetInteractionEnabled`로 토글한다.
- **퀘스트 주도 대사**: `FWxStateTreeTask_PlayDialogue`에 `StartRow`를 지정하면 대상 액터 없이도(나레이션) 로컬 세션에 대사를 연다. 대사를 트리가 소유하므로 같은 NPC라도 단계별로 다른 대사가 가능.
- **연출 정책 조정**: 카메라 FOV·거리·각도·블렌드는 세션 컴포넌트의 `EditDefaultsOnly` 기본값. 주입 컴포넌트라 BP 서브클래스를 만들어 주입 액션에 등록해야 값을 바꾼다.
- **세션 열림/닫힘 관찰**: 시작·종료 델리게이트는 없다. 폰 ASC의 `State.Dialogue` 루즈 태그(WxCore 선언)로 표시하며 UI 매니저가 그 태그를 보고 창을 여닫는다.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 모듈의 심장. 서버→소유 클라 전달, 카메라/포즈 소유, UI와의 분리 정책이 헤더 주석에 다 있다.
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 스키마. 대화가 어떻게 구성되고 어디서 끝나는지의 근본.
3. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` — 대상 측 진입점과 상호작용 계약을 컴포넌트가 드는 이유.

## 관련
- 상위: 퀘스트 ST 태스크로 대사 트리거 [[WxQuest]] · 대화 창 표시 [[WxUI]] · 상호작용 계약·태그 [[WxCore]]

---
*문서 기준 커밋 `1ae8d2f` · 생성일 2026-08-13 · 소스 9파일 — `/readme-writer`로 갱신*
