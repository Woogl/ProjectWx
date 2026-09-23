---
title: "WxWorld — 장치와 상호작용"
category: topic
sources:
  - "raw/notes/2026-09-22-current-world.md"
  - "raw/notes/2026-09-22-current-foundation.md"
  - "raw/notes/2026-09-23-instanced-struct-ftext-default-save-fail.md"
  - "raw/notes/2026-09-23-interaction-list-vm.md"
  - "raw/notes/2026-09-24-device-linked-state-tag.md"
created: 2026-09-22
updated: 2026-09-24
tags: [wx, world]
aliases: ["WxWorld"]
confidence: medium
volatility: warm
verified: 2026-09-24
summary: "WxWorld는 장치 StateTree의 상태 동기화, 로컬 상호작용 탐색, 스포너와 체크포인트 기능을 제공한다."
---

# WxWorld — 장치와 상호작용

WxWorld는 장치 StateTree의 상태 동기화, 로컬 상호작용 탐색, 스포너와 체크포인트 기능을 제공한다.

## 장치 상태의 복제

`AWxDevice`와 `UWxDeviceStateTreeComponent`가 장치 실행의 중심이다. 서버는 상태 태그명·진입 일련번호·상호작용자·선택지 값을 스냅샷으로 발행한다. 발행할 태그는 루트 StateTree 에셋의 활성 상태 중 태그가 있는 가장 깊은 상태에서 고른다. 받는 쪽이 루트 에셋에서만 태그로 상태를 찾으므로, 링크된 에셋 안의 상태 태그는 발행하지 않는다(2026-09-23 수정). 활성 태그가 없거나 이전 태그와 같으면 `PublishState`는 새 스냅샷을 만들지 않는다. 같은 태그의 재진입까지 별도 이벤트로 보존하는 복제 로그는 아니다.

클라이언트는 일련번호가 바로 다음이면 실시간 전이로, 초기 수신·번호 건너뜀은 복원으로 처리한다. 상호작용자 참조가 늦게 해소되어 같은 번호가 다시 통지되면 참조를 갱신하지만 전이를 반복하지 않는다. 태그로 루트 에셋의 상태를 찾아 Critical 전이를 요청하며, 종료된 트리는 먼저 재시작한다.

`FWxDeviceExecutionPolicy`는 초기 진입 또는 복원 플래그를 판별한다. 장치 태스크를 추가할 때 일회성 보상·효과를 복원 중 다시 실행할지, 위치·표시만 맞출지를 구분해야 한다. 스냅샷 하나로 모든 커스텀 태스크의 재생 안전성을 보장하지 않는다.

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

로컬 컨트롤러의 스캐너가 Pawn 주변 쿼리 콜리전을 검색하고 `IWxInteractable`의 자격·선택지를 읽는다. 기존 후보 순서를 보존하고 새 후보를 거리순으로 뒤에 붙여 목록이 매번 뒤섞이지 않게 한다. 선택 값의 의미는 대상 액터가 정의한다.

목록의 행은 대상의 선택지 하나당 하나다. 스캐너는 스캔마다 대상에서 선택지 문구·값을 다시 읽고, 대상·값·문구 중 하나라도 달라졌을 때만 행을 교체한다. 같은 선택지가 남아 있으면 선택을 잇고, 선택지만 바뀌었으면 같은 대상의 첫 행을 잇는다. 행 교체와 선택 변경은 모두 `OnRowsChanged` 하나로 발행한다. 외곽선은 선택을 소유한 스캐너만 건다.

RPC는 선택 액터와 값을 GameplayEvent로 전달하고, WxGame의 Interact 어빌리티가 서버에서 현재 자격·거리·선택지 유효성을 재검사한다. 콜리전이 없는 액터는 인터페이스만 구현해도 감지·거리 검사에 걸리지 않는다. 클라이언트 UI 목록은 실행 권한의 근거가 아니다.

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

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

2026-09-23 편찬: 커밋 `5212bbe3a` 기준으로 엘리베이터 정차 지점 규칙과 FText 기본값 저장 함정을 추가했다. 저장 실패는 DebugGame 에디터에서 MCP로 재현한 결과다. 엘리베이터 버튼 잠금의 인게임 동작은 확인하지 않았다.

2026-09-23 refresh: 원자료 해시 대조로 스캐너 변경(`f98eef471`)을 찾아 HUD 목록 연결과 문구 출처 원칙을 HEAD `7d2a20408` 기준으로 추가했다. 상호작용 목록의 인게임 동작은 확인하지 않았다.

2026-09-24 refresh: 원자료 해시 대조로 장치 상태 태그 발행 범위 변경(`67d288fc5`)을 찾아 HEAD `ca84c9aac` 코드와 대조해 추가했다. 링크된 StateTree를 쓰는 장치 에셋이 있는지와 인게임 동작은 확인하지 않았다.

</details>
