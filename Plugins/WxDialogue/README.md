# WxDialogue — 대화 시스템

> NPC·나레이션 대사를 데이터 테이블로 정의하고, 플레이어와의 대화 세션(대사 진행·연출 카메라·대상 포즈)을 구동한다. 대화의 "의미" 판정(수주·납품 등)은 퀘스트 등 소비 도메인에 맡기고, 이 모듈은 대사의 신원과 진행만 노출한다.

## 책임
**담당**
- 대화 데이터 스키마: `FWxDialogueTableRow`(화자·대사·포즈·다음 행) 한 행 = 대사 한 줄, 테이블 1개 = 대화 1편. `NextRow` 링크로 진행.
- 대화 상대 마킹: `UWxDialogueComponent`가 `IWxInteractable`을 구현해 어떤 액터든 말 걸 수 있는 대상으로 만들고, 시작 행을 보유.
- 대화 세션 진행: `UWxDialogueSessionComponent`(PlayerController 주입)가 현재 노드·라인을 소유하고 대사 넘기기(`Advance`)를 처리.
- 대화 연출: 세션이 전용 대화 카메라 구도 계산·뷰 전환과, 대사가 지목한 대상 포즈(몽타주) 비동기 스트리밍·재생을 직접 든다.
- StateTree 노드 제공: 소비 도메인(퀘스트 등)이 이 모듈을 참조하지 않고도 에셋에서 대화를 관찰(`WaitDialogueCompleted`)·출력(`PlayDialogue`)·NPC 상호작용 토글(`EnableNpcInteraction`)할 수 있게 한다.

**경계 (비담당)**
- 대화의 뜻 해석·기록: 안 한다. "이 대화가 수주다" 같은 판정은 관찰자(퀘스트 [[WxQuest]] ST 태스크)의 몫.
- 대화창 UI: 안 그린다. 대사 변경은 델리게이트로 발행하고, 세션 개폐는 폰 ASC의 `State.Dialogue` 태그로 알린다 — 창을 여닫는 것은 그 태그를 보는 [[WxUI]]의 몫.
- `IWxInteractable`·`State.Dialogue` 태그 정의: [[WxCore]] 소유. 이 모듈은 계약을 구현·소비만.

## 의존성
- **주요 의존**: `WxCore`(IWxInteractable, WxGameplayTags::State_Dialogue) / 엔진: GameplayAbilities(폰 ASC 태그), StateTree(크로스모듈 노드), ModularGameplay(주입), UniversalObjectLocator(NPC 지목).
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 데이터 스키마(대사 1줄 = 행 1개) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `UWxDialogueComponent` | 대화 상대 마킹 + 시작 행 보유(IWxInteractable 구현) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | 플레이어 측 세션 진행·카메라·포즈 소유(PC 주입) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_WaitDialogueCompleted` | 지정 대화 완주 관찰 게이트 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` |
| `FWxStateTreeTask_PlayDialogue` | 트리가 대사를 여는 출력 태스크 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` |
| `FWxStateTreeTask_EnableNpcInteraction` | 지정 NPC 상호작용 토글 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` |

## 확장 포인트 / 규약
- 새 대화: 코드 변경 없이 `FWxDialogueTableRow` 타입 DataTable을 만들고 행을 `NextRow`로 잇는다(`NextRow=None`이면 종료). 대상 NPC의 `UWxDialogueComponent.StartRow`에 시작 행을 지정.
- 대화 상대 만들기: 호스트 액터에 `UWxDialogueComponent`를 붙이고 `SetAreaMesh`로 상호작용 영역 메시를 지정(C++는 생성자, 순수 BP는 디테일 패널). 전용 액터 클래스 불필요.
- 카메라 연출 조정: 세션의 카메라 파라미터는 주입 기본값이 곧 실제 값 — 조정하려면 `UWxDialogueSessionComponent`의 BP 서브클래스를 만들어 Experience 주입 액션에 등록.
- 리플리케이션/권한: v1 싱글/리슨 호스트 전제(서버=소유 클라). 세션 상태는 소유 클라 로컬(서버 검증 없음), 진입은 서버 권위(`StartDialogue`)→Client RPC(`ClientStartDialogue`). 상호작용 토글·크로스모듈 노드는 값 복제 없음.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 세션 진행·카메라·포즈·RPC까지 모듈의 제어 흐름 전체가 헤더 주석에 서술돼 있다. 여기가 심장.
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 스키마. 대화가 무엇으로 이루어지는지가 먼저다.
3. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` — 퀘스트 등 소비 도메인이 대화와 맞물리는 접점(관찰/출력/토글).

## 관련
- 상위: [[WxQuest]](ST 태스크로 대화를 관찰·출력), [[WxUI]](State.Dialogue 태그로 대화창 개폐, 라인 델리게이트 구독). 계약·태그 정의는 [[WxCore]].

---
*문서 기준 커밋 `d3f4ff1` · 생성일 2026-08-08 · 소스 9파일 — `/readme-writer`로 갱신*
