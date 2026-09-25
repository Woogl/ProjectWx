---
title: "WxWorld — 장치와 상호작용"
category: topic
sources:
  - "raw/notes/2026-09-22-current-world.md"
  - "raw/notes/2026-09-22-current-foundation.md"
  - "raw/notes/2026-09-23-instanced-struct-ftext-default-save-fail.md"
  - "raw/notes/2026-09-23-interaction-list-vm.md"
  - "raw/notes/2026-09-24-device-linked-state-tag.md"
  - "raw/notes/2026-09-24-interaction-contract-options-only.md"
  - "raw/notes/2026-09-24-device-statetree-cleanup.md"
  - "raw/notes/2026-09-25-ability-data-on-ga.md"
created: 2026-09-22
updated: 2026-09-25
tags: [wx, world]
aliases: ["WxWorld"]
confidence: medium
volatility: warm
verified: 2026-09-25
summary: "WxWorld는 장치 StateTree의 상태 동기화, 로컬 상호작용 탐색, 스포너와 체크포인트 기능을 제공한다."
---

# WxWorld — 장치와 상호작용

WxWorld는 장치 StateTree의 상태 동기화, 로컬 상호작용 탐색, 스포너와 체크포인트 기능을 제공한다.

## 장치 상태의 복제

`AWxDevice`와 `UWxDeviceStateTreeComponent`가 장치 실행의 중심이다. 서버는 상태 태그·진입 일련번호·상호작용자·선택지 값을 스냅샷으로 발행한다. 발행할 태그는 루트 StateTree 에셋의 활성 상태 중 태그가 있는 가장 깊은 상태에서 고른다. 받는 쪽이 루트 에셋에서만 태그로 상태를 찾으므로, 링크된 에셋 안의 상태 태그는 발행하지 않는다(2026-09-23 수정). 활성 태그가 없거나 이전 태그와 같으면 `PublishState`는 새 스냅샷을 만들지 않는다. 같은 태그의 재진입까지 별도 이벤트로 보존하는 복제 로그는 아니다.

클라이언트는 일련번호가 바로 다음이면 실시간 전이로, 초기 수신·번호 건너뜀은 복원으로 처리한다. 상호작용자 참조가 늦게 해소되어 같은 번호가 다시 통지되면 참조를 갱신하지만 전이를 반복하지 않는다. 태그로 루트 에셋의 상태를 찾아 Critical 전이를 요청하며, 종료된 트리는 먼저 재시작한다. 종료 여부는 `GetStateTreeRunStatus()`로 직접 본다. 순정 `IsRunning`은 스스로 끝난 트리(Tree Succeeded 전이)에도 참이기 때문이다. 장치가 상호작용을 받는지는 대기 노드 등록(`WaitingTask`)만 본다. 트리가 끝나거나 멈추면 대기 노드의 이탈이 등록을 걷는다.

서버의 `InitialState`는 트리를 루트로 시작한 뒤 그 태그 상태로 복원 전이를 요청해 적용한다. 루트 에셋에 그 태그 상태가 없으면 에러 로그를 남기고 루트 상태를 발행한다.

### 복원 판정

`UWxDeviceStateTreeComponent::IsRestoring(Context, Transition)`이 판정한다. 트리 시작(재시작 포함, `SourceStateID` 무효)과 컴포넌트가 스냅샷을 따라 요청한 복원 전이가 복원이다. 장치가 아닌 트리는 트리 시작만 복원이다. 2026-09-24에 별도 구조체 `FWxDeviceExecutionPolicy`에서 옮겼다.

| 태스크 성격 | 복원일 때 | WxWorld 태스크 |
|---|---|---|
| 상태 적용(위치·표시) | 실행한다. 이동은 목표 위치로 즉시 맞춘다. | `ComponentMove`·`SplineMove` |
| 일회성 효과 | 건너뛰고 곧바로 완료한다. | `PlaySound`·`PlayLevelSequence`·`ApplyGameplayEffectToInteractor`·`RecordCheckpoint`·`RespawnSpawners`·`TriggerSpawners`·`TriggerLinkedDevices` |

스냅샷 하나로 모든 커스텀 태스크의 재생 안전성을 보장하지는 않는다. 새 장치 태스크는 위 두 성격 중 하나로 정해 `IsRestoring`을 부른다.

다른 도메인의 태스크는 WxWorld를 참조할 수 없어 트리 시작(`!SourceStateID.IsValid()`)만 복원으로 본다. WxInventory `보상 지급`·`RefillItemCharges`와 WxCombat `몽타주 1회 재생`이 해당한다. 그런데 서버의 `InitialState` 적용은 소스 상태가 있는 전이라서, 이 태스크들은 그것을 실제 진입으로 본다.
- **저작 규칙:** `InitialState`로 지정하는 상태에는 다른 도메인의 일회성 효과를 두지 않는다. 예를 들어 보상 상자를 "열림"으로 배치하면 레벨 시작 때 서버가 보상을 준다.
- 현재 `InitialState`를 쓰는 배치는 피스톤(`Device.Piston.Off`)뿐이라 문제가 드러나지 않는다.
- 순정 해법은 `FStartParameters::SelectStateOverrideArgs`로 지정 상태에서 트리를 시작하는 것이다. 하지만 `UStateTreeComponent::StartTree`는 이 인자를 받지 않는다. 엔진이 이를 열 때까지 보류한다(2026-09-24 사용자 결정).

### 장치 트리의 다른 도메인 태스크

장치 트리에서 쓰더라도 효과가 다른 도메인에 속하면 태스크는 그 도메인에 둔다. 장치의 당사자는 `AWxDevice` 캐스트 대신 `Actor.InteractingCharacter` 바인딩으로 받는다. `InteractingCharacter`가 `VisibleInstanceOnly`인 이유는 바인딩 피커가 편집 가능 프로퍼티만 보여 주기 때문이다.

WxCombat의 `몽타주 1회 재생`(`FWxStateTreeTask_PlayMontageOnce`)이 이 방식이다. 2026-09-24 사용자 제안으로 WxWorld `PlayInteractorMontage`를 대체했다.
- 권위에서 `Target`에게 `UWxAbility_PlayMontageOnce`를 1회 부여·발동하고, 어빌리티가 끝나면 완료한다.
- 권위가 아닌 피어는 서버가 발행하는 다음 상태까지 머문다. 그래서 이 태스크를 둔 상태와 다음 상태는 서로 다른 태그 상태여야 한다.

### 연출 태스크의 피어별 동작

연출 태스크는 모든 피어가 진입할 때 각자 로컬로 재생한다.
- **레벨 시퀀스 재생:** 카메라 컷은 당사자를 조종하는 피어에서만 켠다. 카메라 컷은 그 월드의 첫 로컬 플레이어에게 걸리기 때문이다. 당사자가 없는 트리에서는 카메라를 전환하지 않는다. 재생 자체는 모든 피어가 하므로 상태가 끝나는 시점(서버의 재생 종료)은 달라지지 않는다.
- **사운드 재생:** 라이브 발동에서만 1회 재생하는 원샷이다. 재생 핸들을 남기지 않아 상태를 떠나도 멈추지 않으므로 루프 사운드는 넣지 않는다. 복원 때도 재생하던 `bPlayOnRestore`는 멈출 수 없어서 지웠다.

## 엘리베이터 정차 지점 규칙

엘리베이터의 작동 대기 노드는 수락 규칙 `정차 지점 선택`(`FWxDeviceTriggerRule_SplineStops`)으로 받을 신호와 선택지를 정한다. 정차 지점은 스플라인 포인트이고, 선택지 값은 포인트 번호다. 신호를 보낸 버튼이 탑승칸(`Platform`)에 붙어 있으면 탑승칸 버튼으로, 아니면 밖 호출 버튼으로 구분한다.

| 버튼 | `bWakeOnCall` 끔 | `bWakeOnCall` 켬 (비활성 장치를 깨우는 상태) |
|---|---|---|
| 탑승칸 버튼 | 지금 있는 곳을 뺀 모든 정차 지점. 문구는 규칙의 `StopPrompt`이고 `{0}`에 1부터 센 번호가 들어간다. | 잠긴다. |
| 밖 호출 버튼 | 가장 가까운 정차 지점 하나. 탑승칸이 이미 거기 있으면 잠긴다. 문구는 그 버튼 자신의 작동 대기 노드 `Prompt`다. | 가장 가까운 정차 지점 하나. 탑승칸이 이미 거기 있어도 받아서 제자리에서 문만 연다. |

`ST_Elevator`의 두 작동 대기 노드에는 `StopPrompt`로 `Floor {0}`이 StateTree 값으로 입력되어 있다.

### 규칙 구조체의 문구 필드에는 C++ 기본값을 두지 않는다

수락 규칙처럼 StateTree 노드 안의 인스턴스 구조체에 담기는 `FText`에 C++ 기본값을 두면, 에셋의 값이 그 기본값과 같을 때 저장이 `Unexpected custom version "FortniteMain"` 오류로 중단된다.

- 2026-09-23 `ST_Elevator`에서 재현했다. 기본값과 같은 값은 저장에 실패했다. 기본값과 다른 값, 그리고 기본값을 없앤 빌드에서 입력한 값은 저장에 성공했다.
- 해석(추론): 인스턴스 구조체는 기본값과 같은 프로퍼티를 본문 직렬화에서 생략한다. 그런데 번역 수집은 그 문구를 잡아 패키지 요약 뒤 헤더에 기록하면서 이 커스텀 버전을 처음 쓴다. 엔진의 번역 수집 경로는 추적하지 않았다.

그래서 장치 문구는 작동 대기 노드의 `Prompt`처럼 C++ 기본값 없이 StateTree에서 입력한다. C++ 기본값이 없는 `Prompt`를 수정한 장치 StateTree(`ST_Button` 등)는 정상 저장됐다.

## 상호작용 계약

로컬 컨트롤러의 스캐너는 스캔마다 먼저 Pawn ASC에서 에셋 태그 `Ability.Interact`를 가진 첫 스펙이 지금 발동 가능한지 본다. 발동할 수 없으면 목록과 외곽선을 비운다. 이 판정은 엔진 발동 경로(UE 5.8 `InternalTryActivateAbility`)처럼 스펙의 기본 인스턴스가 있으면 그 인스턴스로 한다(커밋 `0473e201b`). 그다음 Pawn 주변 쿼리 콜리전을 검색하고 `IWxInteractable`의 선택지를 읽는다. 선택지가 빈 대상은 지금 꺼진 것이라 행이 생기지 않는다. 기존 후보 순서를 보존하고 새 후보를 거리순으로 뒤에 붙여 목록이 매번 뒤섞이지 않게 한다. 선택 값의 의미는 대상 액터가 정의한다.

목록의 행은 대상의 선택지 하나당 하나다. 스캐너는 스캔마다 대상에서 선택지 문구·값을 다시 읽고, 대상·값·문구 중 하나라도 달라졌을 때만 행을 교체한다. 같은 선택지가 남아 있으면 선택을 잇고, 선택지만 바뀌었으면 같은 대상의 첫 행을 잇는다. 행 교체와 선택 변경은 모두 `OnRowsChanged` 하나로 발행한다. 외곽선은 선택을 소유한 스캐너만 건다.

RPC는 선택 액터와 값을 GameplayEvent로 전달하고, WxGame의 Interact 어빌리티가 서버에서 현재 거리와 선택지 유효성을 재검사한다. 자격을 잃은 대상은 선택지가 비어 여기서 거절된다. 콜리전이 없는 액터는 인터페이스만 구현해도 감지·거리 검사에 걸리지 않는다. 클라이언트 UI 목록은 실행 권한의 근거가 아니다.

### HUD 목록 연결

WxGame의 `UWxViewModel_InteractionList`가 스캐너를 구독한다. 신호가 올 때마다 스캐너에서 목록과 선택을 읽어 행 VM(WxUI `UWxViewModel_Interaction`)을 전부 다시 만든다(2026-09-23 사용자 결정, 커밋 `f98eef471`).
- 행 VM은 `Prompt`·`bSelected`만 가지며 만들어진 뒤 바뀌지 않는다. 선택을 목록 VM과 행에 따로 동기화하지 않는다.
- 대가로 선택을 바꿀 때마다 ListView 엔트리가 새로 붙는다. 선택 전환 애니메이션을 넣으려면 선택 신호를 다시 나눠야 한다.
- 목록 VM과 행 VM의 두 클래스 구조는 유지한다. 하나로 합치는 안은 한 클래스가 두 역할을 맡고 모듈 이동이 필요해 기각했다.

리졸버는 위젯 소유 PC에서 `FindComponentByClass`로 스캐너를 찾아 목록 VM에 넘기고, `DestroyInstance`에서 구독을 끊는다. 엔진 Create Instance는 `NewObject`만 호출하므로 리졸버 없이는 스캐너를 넘기지도, VM을 정리하지도 못한다. 이 연결은 스캐너가 `AWxPlayerController` 생성자 컴포넌트라 위젯보다 먼저 있다는 전제에 기댄다. 스캐너를 Experience 등에서 나중에 주입하는 구조로 바꾸면 늦은 도착 처리가 다시 필요하다. 예전 `OnAnyScannerReady` 관찰이 그 용도였고 이번에 제거했다.

빌드와 `WBP_InteractionList`·`WBP_Interaction` 컴파일은 확인했다. 목록 표시·휠 선택·선택지 실행·리스폰 후 동작은 인게임에서 확인하지 않았다.

### 상호작용 문구의 출처

문구는 상호작용했을 때 실제로 일어날 행동의 주인이 갖는다(2026-09-23 사용자 결정). 모든 문구를 StateTree로 모으는 안은 기각했다.

| 대상 | 문구의 주인 |
|---|---|
| 장치 | StateTree(작동 대기 노드 `Prompt`, 엘리베이터 규칙 `StopPrompt`) |
| 대화 액터 | 대화 컴포넌트 |
| 픽업 | 아이템 데이터(실행 중 스폰되는 드랍이라 StateTree가 맞지 않는다) |
| 피니시 | 피니시 어빌리티 |

문구에 키 표기(`[F]` 등)를 넣지 않는다. 키 아이콘은 `WBP_Interaction`의 `CommonActionWidget`이 입력 액션 데이터(`DT_InputActions`의 `Interact` 행)로 표시한다.

## 스폰과 체크포인트

스포너는 별도 Spawnable 경로와 공용 `IWxSpawnable` 처치 통지를 사용한다. 순찰 경로와 스폰/빙의 초기화는 WxAI·WxGame과 함께 확인한다.

CheckpointSubsystem은 Standalone에서만 레벨 패키지와 위치·회전을 보관한다. PIE 접두사를 제거해 같은 레벨인지 검사하고 다른 레벨에서는 반환하지 않는다. 디스크 세이브나 멀티플레이 체크포인트 저장 시스템이 아니다. 부활과 스포너 재생성 조립은 WxGame에 있다.

진입점: [장치 동기화](../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp), [상호작용 스캐너](../../../Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp), [상호작용 목록 VM](../../../Source/WxGame/MVVM/WxViewModel_InteractionList.cpp), [체크포인트](../../../Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSubsystem.cpp).

## 관련 문서

- [[ai|WxAI — AI 인지와 행동]] ([WxAI — AI 인지와 행동](../topics/ai.md))
- [[combat|WxCombat — 전투 시스템]] ([WxCombat — 전투 시스템](../topics/combat.md))
- [[combat-finisher|그로기 피니시와 뒤잡]] ([그로기 피니시와 뒤잡](../concepts/combat-finisher.md))
- [[editor-tools|편집기 도구 — WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer]] ([편집기 도구 — WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer](../references/editor-tools.md))
- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[inventory|WxInventory — 아이템 소유와 사용]] ([WxInventory — 아이템 소유와 사용](../topics/inventory.md))
- [[quests|WxQuest — 퀘스트 실행과 저널]] ([WxQuest — 퀘스트 실행과 저널](../topics/quests.md))
- [[ui|WxUI — 화면 레이어와 표시 수명]] ([WxUI — 화면 레이어와 표시 수명](../topics/ui.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-world.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)
- [근거 3](../../raw/notes/2026-09-23-instanced-struct-ftext-default-save-fail.md)
- [상호작용 목록 VM과 문구 출처](../../raw/notes/2026-09-23-interaction-list-vm.md)
- [장치 상태 태그는 루트 에셋에서만 발행](../../raw/notes/2026-09-24-device-linked-state-tag.md)
- [장치 StateTree 정리](../../raw/notes/2026-09-24-device-statetree-cleanup.md) — 복원 판정 이동, InitialState 제약, 몽타주 태스크 WxCombat 이관, 연출 태스크
- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — 스캐너의 상호작용 발동 판정을 스펙의 기본 인스턴스 기준으로 변경

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

2026-09-23 편찬: 커밋 `5212bbe3a` 기준으로 엘리베이터 정차 지점 규칙과 FText 기본값 저장 함정을 추가했다. 저장 실패는 DebugGame 에디터에서 MCP로 재현한 결과다. 엘리베이터 버튼 잠금의 인게임 동작은 확인하지 않았다.

2026-09-23 refresh: 원자료 해시 대조로 스캐너 변경(`f98eef471`)을 찾아 HUD 목록 연결과 문구 출처 원칙을 HEAD `7d2a20408` 기준으로 추가했다. 상호작용 목록의 인게임 동작은 확인하지 않았다.

2026-09-24 refresh: 원자료 해시 대조로 장치 상태 태그 발행 범위 변경(`67d288fc5`)을 찾아 HEAD `ca84c9aac` 코드와 대조해 추가했다. 링크된 StateTree를 쓰는 장치 에셋이 있는지와 인게임 동작은 확인하지 않았다.

2026-09-24 refresh(2차): 커밋 추적으로 장치 StateTree 정리 8건(`b32c1f622`~`70495c0e7`)을 찾아 HEAD `142fab5d6` 코드와 대조해 복원 판정·InitialState 제약·다른 도메인 태스크·연출 태스크를 반영했다. 빌드는 `36fbb4371`까지만 기록이 있고 인게임 동작은 확인하지 않았다.

2026-09-25 refresh: 스캐너의 상호작용 발동 판정(`CanActivateInteract`, 커밋 `0473e201b`)을 HEAD `d63ce0630` 코드와 UE 5.8 `InternalTryActivateAbility`와 대조해 추가했고, 상호작용 목록의 인게임 동작은 확인하지 않았다.

</details>
