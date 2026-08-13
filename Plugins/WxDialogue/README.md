# WxDialogue — 대화 시스템

> DataTable 로 저작한 대사를 NPC 상호작용·퀘스트 StateTree 로 재생하는 대화 시스템. 대사 표시/카메라 연출/포즈 재생을 담당하고, 대사의 뜻(수주·납품 판정)은 관찰하는 소비자에게 맡긴다.

## 책임
**담당**
- 대화 데이터 스키마: `FWxDialogueTableRow`(화자·대사·포즈·NextRow) 로 대화 1편 = 테이블 1개.
- 대화 상대 정의: `UWxDialogueComponent` 를 붙인 액터를 말 걸 수 있는 대상으로 만들고(`IWxInteractable` 구현), 상호작용 활성 토글.
- 세션 진행: `UWxDialogueSessionComponent`(PlayerController 주입)가 현재 노드·라인을 소유하고 대사 넘기기, 대화 전용 카메라 구도, 대상 포즈 스트리밍/재생.
- StateTree 태스크: 퀘스트가 대화를 여는 `Play Dialogue`.

**경계 (비담당)**
- 대사 의미 판정(수주·납품 등) — 대화는 뜻을 해석하지 않고, 관찰하는 [[WxQuest]] 데이터가 판정한다.
- 대화창 UI 표시 — 델리게이트/`State.Dialogue` 태그만 발행하고 창 여닫기는 [[WxUI]] 가 맡는다.
- 상호작용 감지·프롬프트 표시 스캐너 — `IWxInteractable` 계약만 구현하고 감지 파이프라인은 [[WxWorld]] 측이다.
- 상호작용 여닫기 태스크 — 잠금/해제는 계약의 `SetInteractionEnabled` 로 받기만 하고, 그것을 부르는 `Enable Interaction` 태스크는 [[WxWorld]] 소유다.

## 의존성
- **주요 의존**: `WxCore`(`IWxInteractable`, `WxGameplayTags`). 엔진: StateTree, GameplayAbilities(ASC 루즈 태그), ModularGameplay(컨트롤러 주입), UniversalObjectLocator.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 노드 1행 스키마(화자·대사·포즈·NextRow) | `Plugins\WxDialogue\Source\WxDialogue\Public\WxDialogueTableRow.h` |
| `UWxDialogueComponent` | 대화 상대 컴포넌트, `IWxInteractable` 구현·상호작용 토글 | `Plugins\WxDialogue\Source\WxDialogue\Public\WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | PC 주입 세션, 진행·카메라·포즈 소유 | `Plugins\WxDialogue\Source\WxDialogue\Public\WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 세션에 지정 대사를 열고 완료까지 대기 | `Plugins\WxDialogue\Source\WxDialogue\Public\WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- 대화 저작: `FWxDialogueTableRow` 로 DataTable 생성 → `UWxDialogueComponent::StartRow` 또는 태스크의 `StartRow` 로 시작 노드 지정. NextRow=None 이 종료.
- 진입 경로 둘: 액터가 화자면 상호작용이 `StartDialogue(Dialogue)`, 액터 없이 대사를 고르면(독백·퀘스트) `StartDialogueRow(Row, Target)`.
- 카메라 연출 조정: 세션 컴포넌트의 `EditDefaultsOnly` 카메라 파라미터를 BP 서브클래스로 오버라이드해 주입 액션에 등록.
- 세션 개폐 관찰: 폰 ASC 의 `State.Dialogue` 루즈 태그(WxCore 선언)로 알리며, 라인 변경은 `OnLineChanged` 델리게이트.
- 전제: v1 싱글/리슨 호스트(서버=소유 클라). 세션 상태는 소유 클라 로컬, 복제 없음. 태스크는 0번 컨트롤러를 쓴다.

## 여기서부터 읽어라
1. `Plugins\WxDialogue\Source\WxDialogue\Public\WxDialogueSessionComponent.h` — 세션이 진행·카메라·포즈를 어떻게 소유·발행하는지가 이 모듈의 핵심 서사.
2. `Plugins\WxDialogue\Source\WxDialogue\Public\WxDialogueTableRow.h` — 대화 데이터 모델을 먼저 잡으면 나머지가 읽힌다.
3. `Plugins\WxDialogue\Source\WxDialogue\Public\WxDialogueComponent.h` — 상호작용 계약을 컴포넌트가 드는 이유와 액터 진입 경로.

## 관련
- 상위: [[WxQuest]](태스크로 대화를 연다), [[WxWorld]](상호작용 감지·상호작용 대기 게이트), [[WxUI]](대화창 표시)

---
*문서 기준 커밋 `dfd2174` · 생성일 2026-08-12 · 소스 13파일 — `/readme-writer`로 갱신*
