# WxDialogue — 대화 시스템

> NPC·나레이션 대사를 DataTable로 구동하고, 상호작용으로 시작된 대화 세션을 플레이어 측에서 진행하며 대화 카메라·NPC 포즈를 연출한다. 대사의 신원(현재 행)만 노출하고 그 의미 해석은 소비자에게 맡긴다.

## 책임
**담당**
- 대화 데이터 정의: `FWxDialogueTableRow`(화자·대사·포즈·NextRow)로 한 테이블 = 대화 1편.
- 대화 상대 선언: `UWxDialogueComponent`가 액터에 붙어 상호작용 대상(`IWxInteractable`)이 되고 시작 노드를 보유.
- 세션 진행: `UWxDialogueSessionComponent`가 PlayerController에 주입되어 노드 전환·대사 발행·종료를 소유.
- 연출: 대화 전용 카메라 구도, 대사별 NPC 포즈 비동기 스트리밍/재생.
- StateTree 진입점: `FWxStateTreeTask_PlayDialogue`로 퀘스트/AI가 액터 없이 대사를 재생.

**경계 (비담당)**
- 상호작용 감지·발동, `IWxInteractable`·`State.Dialogue` 태그 정의 → [[WxCore]].
- 대화 창 여닫기·대사 렌더링(뷰모델은 델리게이트/태그 관찰) → [[WxUI]].
- 대화의 의미 판정(퀘스트 수주 등)·태스크 조립 → [[WxQuest]] 등 관찰자 측.
- 태그 차단(대화 중 이동/상호작용 봉쇄)은 상호작용 어빌리티의 `State.Dialogue` 차단 태그가 담당.

## 의존성
- **주요 의존**: `WxCore`(`IWxInteractable`, `WxGameplayTags::State_Dialogue`), GameplayAbilities(`UAbilitySystemComponent` 루즈 태그), StateTree(태스크), ModularGameplay(컨트롤러 주입), UniversalObjectLocator.
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 데이터 행(화자·대사·`TargetPose`·`NextRow`). 전체 시스템이 이 스키마를 구동 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `UWxDialogueComponent` | 액터에 붙는 대화 상대. `IWxInteractable` 구현, 시작 행만 보유하고 진행은 세션에 위임 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `UWxDialogueSessionComponent` | PC 주입 세션 소유자. 노드 전환·카메라·포즈·`OnLineChanged`/`OnDialogueEnded` 발행 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 액터 없이 세션에 대사를 여는 ST 태스크. 종료까지 Running | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- 새 대화 추가: `FWxDialogueTableRow` RowType의 DataTable을 만들고 행을 `NextRow`로 연결(종료는 `NextRow=None`). 모든 행의 `Line`은 채워야 하며, 비면 잘못된 행으로 보고 경고 후 접힌다.
- 대화 상대 만들기: 액터에 `UWxDialogueComponent`를 붙이고 `StartRow`·`SpeakerName`·`AreaMesh`(쿼리 콜리전 켠 상호작용 영역)를 지정. 전용 C++ 액터 클래스 불필요.
- 퀘스트/AI 대사: StateTree에 `대화 재생`(`FWxStateTreeTask_PlayDialogue`) 태스크를 두고 `StartRow` 지정 — 대상 액터 없이 로컬 플레이어 세션에서 재생.
- 진입 경로 두 갈래: 액터 상호작용은 `StartDialogue(UWxDialogueComponent*)`, 액터 아닌 쪽은 `StartDialogueRow(handle, Target)`. 둘 다 소유 클라의 `ClientStartDialogue` RPC로 수렴.
- 관찰 규약: 세션 개폐는 폰 ASC의 `State.Dialogue` 루즈 태그, 대사 변경은 `OnLineChanged`, 종료는 일회성 `OnDialogueEnded`. 현재 대사 신원은 `GetCurrentRowHandle()`.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 데이터 모델. 대화 전체가 이 행 스키마로 굴러가므로 먼저 본다.
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 세션 소유자. 진입/RPC/카메라/포즈/발행 흐름과 네트워크 전제가 헤더 주석에 정리돼 있다.
3. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` — 대화 상대가 어떻게 상호작용에서 세션으로 위임하는지.

## 관련
- 상위: [[WxQuest]]·[[WxAI]](ST 태스크로 대사 재생), [[WxUI]](태그·델리게이트 관찰로 대화 창 구동), [[WxCore]](상호작용 계약·태그 제공).

---
*문서 기준 커밋 `6f60b14` · 생성일 2026-08-14 · 소스 9파일 — `/readme-writer`로 갱신*
