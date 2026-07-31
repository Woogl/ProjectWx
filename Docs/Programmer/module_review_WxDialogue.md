# WxDialogue — 코드 리뷰

> 모듈 경계와 책임 분리는 여전히 깔끔하다 — 의존은 `WxCore`/엔진 플러그인뿐이고, 세션이 UI 를 모른 채 태그·델리게이트로만 말하며, 이전 리뷰의 소프트락(Choices 반쪽 구현)은 데이터 모델이 단순해지며 사라졌다. 다만 세션의 수명 관리가 여전히 `Advance()` 한 줄기에 매달려 있어, 대화가 겹치거나 밖에서 끊길 때 빠져나올 길이 없다. 커버리지: 소스 10개 전부 읽었고 세션 컴포넌트 cpp/h 와 StateTree 노드를 정독했으며, 실제 발현을 확인하려 소비처(`UWxUIManagerSubsystem`·`UWxViewModel_Dialogue`·`UWxAbility_Interact`)까지 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🔴 대화가 겹쳐 시작되면 State.Dialogue 카운트가 남아 영구 소프트락이 된다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:86-110`
- **범주**: 버그/정확성
- **문제**: `ClientStartDialogue_Implementation` 이 이미 활성 세션인지 보지 않고 무조건 새 세션으로 덮어쓴다. `AddLooseGameplayTag`(`:105`)는 카운트 +1, `EndDialogue` 의 `RemoveLooseGameplayTag`(`:142`)는 -1 이므로, 두 번 시작되면 대화가 끝난 뒤에도 `State.Dialogue` 카운트 1 이 폰 ASC 에 영구히 남는다.
  귀결이 나쁘다. `UWxUIManagerSubsystem` 은 이 태그를 `EGameplayTagEventType::NewOrRemoved` 로 듣는데(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:300`), 그 이벤트는 0↔비0 전이에서만 발화하므로 2→1 은 신호가 없다 → `CloseDialogueScreen`(`:320`)이 영영 불리지 않아 대화 창이 대사 없이 열린 채 남는다. 동시에 `UWxAbility_Interact` 의 `ActivationBlockedTags`(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:37`)가 계속 닫혀 상호작용·스캐너 프롬프트가 모두 죽는다. 세션이 이미 닫혀 `Advance()` 는 즉시 반환하므로(`:47-50`) 플레이어가 스스로 풀 방법이 없다.
  겹침 경로는 실재한다. `FWxStateTreeTask_PlayDialogue::EnterState`(`Private/WxDialogueStateTreeNodes.cpp:119`)가 활성 세션 여부를 보지 않고 `StartDialogueRow` 를 호출한다 — 무전·독백처럼 트리가 스스로 여는 대사라 상호작용 차단 태그의 게이트를 아예 거치지 않는다. 플레이어가 NPC 와 대화 중일 때 퀘스트 트리가 `Play Dialogue` 상태로 넘어가면 그대로 발현한다.
  같은 함수의 실패 경로(`:89-94`)도 `CurrentStartRow`/`CurrentRowName` 만 되돌리고 `CurrentRow` 는 직전 세션의 행을 계속 가리켜, `HasActiveDialogue()` 는 참인데 테이블은 비어 있는 어긋난 상태를 남긴다. 이 상태에선 `PlayDialogue` 의 성공 판정(`Private/WxDialogueStateTreeNodes.cpp:122`)이 열리지도 않은 대화를 열렸다고 보고, `Tick` 이 영원히 `Running` 이라 퀘스트가 멈춘다.
  덤으로 겹칠 때 `DialogueCamera`(`:193`)가 덮여, 앞 세션 카메라는 `SetLifeSpan` 을 못 받고 컨트롤러가 사라질 때까지 월드에 남는다.
- **제안**: `ClientStartDialogue_Implementation` 진입부에서 `HasActiveDialogue()` 면 `EndDialogue()` 로 앞 세션을 정리하고 시작한다(또는 새 시작을 거부). 태그는 `AddLooseGameplayTag` 대신 `SetLooseGameplayTagCount(WxGameplayTags::State_Dialogue, 1)` 로 두면 카운트 누수가 원천 차단된다. 실패 경로에선 `CurrentRow` 도 함께 `nullptr` 로 되돌린다. `FWxStateTreeTask_PlayDialogue::EnterState` 쪽에도 "이미 대화 중이면 Failed(또는 대기)" 정책을 명시해 두면 겹침 의도가 데이터에서 드러난다.
- **확신도**: 높음(메커니즘은 코드로 확정. 동시 진입 빈도는 퀘스트 트리 조립에 달렸다)

### 2. 🟡 세션을 밖에서 끊을 경로가 없다 — 굳으면 카메라·태그가 함께 묶인다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:117`, `Private/WxDialogueSessionComponent.cpp:45-59`
- **범주**: 설계/구조
- **문제**: `EndDialogue()` 는 private 이고 도달 경로가 `Advance()` 하나뿐이다. 즉 "뷰가 대사를 넘긴다"는 단 하나의 사건 말고는 세션이 끝나지 않으며, `EndPlay`/`UninitializeComponent`/빙의 변경 훅도 없다.
  반대편은 그렇지 않다. `UWxUIManagerSubsystem::WatchPawnTags` 는 빙의가 바뀌면 무조건 `CloseDialogueScreen()`(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:288`)으로 대화 창을 닫는다. 대화 중 폰이 교체되면 창은 닫히는데 세션은 활성인 채 남고, `Advance` 를 부를 뷰가 사라져 `HasActiveDialogue()` 가 영영 참이 된다 — `Wait Dialogue Completed`(세션 종료로 완주 판정)와 `Play Dialogue`(세션 종료로 성공 판정) 두 태스크가 함께 멈춘다. 이때 뷰 타겟도 대화 카메라에 머물고, 태그는 이전 폰 ASC 에 남는다(`TaggedAbilitySystem` 이 그쪽을 붙잡고 있다).
- **제안**: 공개 취소 경로(`CancelDialogue()` 등)를 열고, 컴포넌트 종료(`UninitializeComponent`)와 빙의 변경(`APlayerController::OnPossessedPawnChanged`) 시 세션을 정리한다. 최소한 `EndDialogueCamera()` 만이라도 컴포넌트 정리 시점에 불러 뷰 타겟과 스폰 카메라를 되돌린다.
- **확신도**: 중간(폰 교체 빈도는 콘텐츠에 달렸으나, 비대칭 자체는 코드로 확정)

### 3. 🟡 세션의 데이터 오류가 전부 무음으로 사라진다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:26-29`, `:37-40`, `:52-56`, `:112-119`
- **범주**: 버그/정확성
- **문제**: 세션의 실패 경로가 하나도 로그를 남기지 않는다. `StartRow` 미지정 NPC 는 F 를 눌러도 아무 일이 없고(`:37-40`), 행 이름 오타나 `Line` 이 빈 행은 `EnterRow` 실패로 조용히 걸러진다(`:112-119`). 특히 `Advance()` 는 `EnterRow(NextDialogue)` 실패를 정상 종료와 같은 경로로 처리하므로(`:52-56`), `NextDialogue` 오타가 "대화가 이유 없이 중간에 끊김"으로만 나타나 단서가 없다. `FindRow` 의 ContextString 경고는 행이 아예 없을 때만 뜨고, 대사가 빈 행과 `StartRow` 미지정은 흔적조차 없다.
  모듈은 `LogWxDialogue` 를 이미 갖고 있고 StateTree 노드는 잘 쓰고 있는데(`Private/WxDialogueStateTreeNodes.cpp:45`,`:107`,`:114`,`:124`), 정작 디자이너가 DT 를 직접 편집하는 세션 쪽만 비어 있다.
- **제안**: 위 네 지점에 `UE_LOG(LogWxDialogue, Warning, ...)` 로 테이블명·행 이름을 남긴다. 특히 `Advance` 의 "다음 행 해석 실패"는 정상 종료와 구분해 찍어야 의미가 있다.
- **확신도**: 높음

### 4. 🟢 CurrentRow 가 DataTable 행 메모리를 세션 내내 원시 포인터로 붙든다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:139`
- **범주**: 성능/안전
- **문제**: `CurrentStartRow` 가 `UDataTable` 객체는 GC 로부터 지키지만, 행 메모리는 `UDataTable::EmptyTable()`(리임포트·에디터 편집·`OnPostDataImport`)에서 해제된다. PIE 중 디자이너가 대화 테이블을 건드리면 다음 `Advance()`/`GetCurrentLine()` 이 해제된 메모리를 읽는다. 헤더 주석이 "세션 중에만 유효"라 못 박은 인지된 제약으로 보이나, 이미 `CurrentRowName` 을 따로 들고 있어 회피 비용이 사실상 0 이다.
- **제안**: `CurrentRow` 를 지우고 접근 시점마다 `CurrentStartRow.DataTable->FindRow<FWxDialogueTableRow>(CurrentRowName, ...)` 로 해석한다. 조회는 대사를 넘길 때와 표시할 때뿐이라 비용이 없고, `HasActiveDialogue()` 는 `CurrentRowName.IsNone()` 으로 대체된다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 5. 🟢 헤더 인라인 함수 정의 (코딩 규칙 6)
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:56`, `Public/WxDialogueComponent.h:21`, `Public/WxDialogueStateTreeNodes.h:63`, `:101`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 6("인라인 함수 정의를 금지한다")에 대한 위반이다. `HasActiveDialogue`·`GetStartRow` 는 순수한 사소 게터고, `GetInstanceDataType` 두 건은 StateTree 순정 관용구다.
- **제안**: 게터 두 건은 cpp 로 내리면 끝난다. `GetInstanceDataType` 은 저장소의 모든 StateTree 노드 헤더가 같은 모양이라(`WxQuestStateTreeNodes.h`·`WxGimmickStateTreeNodes.h` 등) 이 모듈만 고칠 일이 아니다 — 규칙의 예외로 명시하든 전역으로 정리하든 프로젝트 차원의 결정이 먼저다.
- **확신도**: 높음(규칙 문언 기준). 다만 코드베이스 전반의 관례라 이 모듈 단독 조치는 효과가 작다

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxNpc.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/README.md`
- **미검토 / 한계**: 발견 근거 검증용으로만 모듈 밖 파일을 읽었다(`Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Plugins/WxUI/.../WxUIManagerSubsystem.cpp`, `Source/WxGame/.../WxAbility_Interact.cpp`) — 리뷰 대상은 아니다. `WBP_DialogueScreen`·`BP_Npc`·`DT_*` 대화 테이블의 실제 내용은 uasset 이고 `WxBlueprintSnapshot/Snapshots/` 미러도 현재 없어 확인하지 못했다. 발견 1 의 최종 체감(입력 잠김 여부)은 위젯의 입력 모드에 달려 있어 C++ 근거로만 적었다. 카메라 구도 수식(`BeginDialogueCamera` 의 축·측면 판정)은 논리적으로만 따라갔고 실플레이 검증은 하지 않았다.

---
*문서 기준 커밋 `c37b6fa6` · 리뷰일 2026-07-31 · 소스 10파일 — `/module-review`로 갱신*
