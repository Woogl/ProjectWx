# WxDialogue — 대화 시스템

> NPC·오브젝트에 말을 걸었을 때 진행되는 대사 세션을 담당한다. 대사 데이터(DataTable)를 한 줄씩 따라가며 화자/대사를 발행하고, 그동안의 대화 카메라와 대상 포즈를 세운다.

## 책임
**담당**
- 대화 데이터 포맷(`FWxDialogueTableRow`: 대사 한 줄 = 행 하나, 대화 1편 = 테이블 1개)과 `NextRow` 체인 순회
- 세션 수명: 열기(`StartDialogue` / `StartDialogueRow`) → `Advance()` → 종료, 그리고 종료 시 `OnDialogueEnded` 1회 발화
- 말 걸 수 있는 대상의 공통 호스트(`AWxDialogueActor`)와 그 대상이 어느 행에서 시작하는지의 보유(`UWxDialogueComponent`)
- 대화 전용 카메라 액터 생성·뷰 타겟 전환·복귀 (컨트롤러에 붙어 있어 뷰 타겟에 손이 닿는 유일한 자리)
- 대사별 포즈 몽타주 비동기 스트리밍 및 대상 스켈레탈 메시에 재생
- 세션 개폐 사실을 폰 ASC 의 `State.Dialogue` loose 태그로 발행

**경계 (비담당)**
- 대화 창 위젯의 개폐·표시 — `State.Dialogue` 태그 전이를 듣는 [[WxUI]] 의 UI 매니저가 판단하고, 대사 바인딩은 [[WxGame]] 의 `UWxViewModel_Dialogue` 가 `OnLineChanged` 를 받아 처리한다. 모듈은 UI 타입을 전혀 모른다
- 상호작용 감지·입력·차단 — `IWxInteractable` 계약 자체는 [[WxCore]] 가 정의하고, `State.Dialogue` 로 상호작용 어빌리티를 막는 것은 [[WxGame]] 쪽이다
- "말을 걸 수 있는가"의 조건 판정(`CanInteract`) — 퀘스트 대기 상태를 봐야 하므로 [[WxGame]] 의 `AWxNpc` 가 답한다
- 대사가 무슨 의미인지(퀘스트 수주·보상 등)의 해석 — 종료를 기다린 쪽([[WxQuest]] 등)이 판정한다. 대화는 기록을 남기지 않는다
- 네이티브 태그 선언 — `State.Dialogue` 는 [[WxCore]] 의 `WxGameplayTags.h` 소유
- NPC 외형/골격 구성 — [[WxGame]] 의 `AWxNpc`

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxDialogueSessionComponent` | 모듈의 심장. 나머지 타입은 전부 이곳에 진입하거나 이곳이 부리는 것이다. PC 측에 붙어 세션 상태·카메라·포즈를 한꺼번에 든다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `FWxDialogueTableRow` | 세션이 순회하는 유일한 데이터 형식. 대화 저작은 이 구조체를 쓰는 DataTable 을 만드는 일이다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `AWxDialogueActor` | 상호작용 계약(`IWxInteractable`)을 세션으로 넘겨 주는 어댑터. 루트를 세우지 않는 Abstract 라 몸통은 파생이 만든다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h` |
| `UWxDialogueComponent` | 대상 쪽이 가진 유일한 상태(시작 행·화자 이름). 세션 진행은 들지 않는다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `FWxStateTreeTask_PlayDialogue` | 액터를 거치지 않는 두 번째 진입 경로. 대사를 트리가 소유해 퀘스트 단계별 대사·독백을 낸다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h` |

## 확장 포인트 / 규약
- **말 걸 수 있는 대상 추가**: `AWxDialogueActor` 를 상속해 루트/메시를 직접 세우고, 포즈를 쓰려면 `GetPoseMesh()` 를 오버라이드해 스켈레탈 메시를 돌려준다. `UWxDialogueComponent` 만 아무 액터에 붙여서는 말을 걸 수 없다 — 상호작용 계약은 액터 전용이다.
- **대화 1편 추가**: `FWxDialogueTableRow` 행 구조체 DataTable 을 만들고 행을 `NextRow` 로 잇는다. 마지막 행은 `NextRow = None`. `Line` 이 빈 행은 잘못된 조립으로 보고 경고와 함께 세션을 접는다. `TargetPose` 는 소프트 참조라 비워 두면 직전 포즈가 유지된다.
- **두 진입 경로**: ① 상호작용 — `AWxDialogueActor::OnInteracted` → `UWxDialogueComponent::StartDialogueWith` → 상대 컨트롤러에서 세션 컴포넌트를 찾아 `StartDialogue`. ② StateTree — `FWxStateTreeTask_PlayDialogue::EnterState` 가 0번 컨트롤러 세션에 `StartDialogueRow(행, nullptr)`. ②는 대상이 없어 카메라가 플레이어에 머물고 포즈도 얹히지 않으며, 상호작용 차단 태그 게이트를 거치지 않으므로 세션이 겹쳐 열릴 수 있다(앞 세션을 먼저 접는다).
- **UI 와의 규약**: 세션은 시작·종료 델리게이트를 UI 에 주지 않는다. 열림/닫힘은 `State.Dialogue` 태그 전이로, 대사 갱신은 `OnLineChanged` 로 간다. 첫 대사는 브로드캐스트되지 않고 관찰자가 `GetCurrentSpeaker()`/`GetCurrentLine()` 으로 pull 해 시드한다. 넘기기는 뷰가 `Advance()` 를 호출한다.
- **ASC 전제**: 폰 ASC 가 없으면 세션을 아예 열지 않는다(태그를 올릴 곳이 없으면 창을 띄울 방법도 없다). 같은 이유로 빙의가 바뀌면 진행 중 세션을 접는다.
- **권한 모델(최대 4인 멀티)**: 서버 권위에서 진입해 `ClientStartDialogue` Client RPC 로 소유 클라에 넘긴다(대화 대상은 비소유 액터라 직접 RPC 를 쏠 수 없다). 그래서 세션 컴포넌트는 복제 컴포넌트이며 PC 의 기본 서브오브젝트다. 진행 상태 자체는 표시 전용 로컬 상태로 서버 검증이 없다 — v1 싱글/리슨 호스트(소유 클라=권위 동일 머신) 전제이며, StateTree 태스크가 `StartDialogueRow` 직후 `HasActiveDialogue()` 를 곧바로 읽는 것도 이 전제 위에 있다.
- **행 포인터 비캐시 규약**: 진행 중 행은 매번 테이블에서 되찾는다(`FindCurrentRow`). 에디터 재임포트가 행 버퍼를 통째로 갈아끼우므로 포인터를 붙잡아 두면 안 된다. 테이블 객체 자체는 `CurrentStartRow` 핸들의 강참조로 세션 동안 유지된다.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 클래스 주석이 모듈의 설계 근거(왜 PC 에 붙는지, 왜 UI 를 모르는지, 왜 카메라·포즈만 직접 드는지)를 전부 담고 있다. 여기만 읽으면 나머지는 배치의 문제다.
2. `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — `ClientStartDialogue_Implementation` → `EnterRow` → `Advance` → `EndDialogue` 순으로 따라가면 세션 한 판이 그대로 보인다. 카메라 구도 계산(`BeginDialogueCamera`)과 포즈 스트리밍(`ApplyCurrentPose`)은 그 뒤에 봐도 된다.
3. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 세션이 소비하는 데이터의 전부. 짧다.
4. `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp` — 상호작용에서 세션까지 어떻게 건너가는지(대상 → Interactor 폰 → 컨트롤러 → 세션)를 보여 주는 15줄.

## 관련
- 상위: [[WxGame]] — `AWxPlayerController` 가 세션 컴포넌트를 생성자에서 붙이고, `AWxNpc` 가 `AWxDialogueActor` 를 상속하며, `UWxViewModel_Dialogue` 가 `OnLineChanged`/`Advance()` 를 잇는다
- [[WxUI]] — `State.Dialogue` 태그 전이로 대화 창을 여닫는다
- [[WxCore]] — `IWxInteractable` 계약과 `WxGameplayTags::State_Dialogue` 선언
- StateTree 트리(퀘스트 등) — `대화 재생` 태스크로 이 모듈을 호출하고, 종료(`OnDialogueEnded`)를 기다려 그 의미를 판정한다

---
*문서 기준 커밋 `047197a` · 생성일 2026-09-16 · 소스 11파일 — `/readme-writer`로 갱신*
