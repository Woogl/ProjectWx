# WxDialogue — 코드 리뷰

> 신규 모듈치고 경계가 깔끔하다 — 의존은 `WxCore`/`GameplayAbilities` 뿐이고, 세션이 UI 를 모르며 델리게이트로만 말하는 구조, 폰 교체 대비 `TWeakObjectPtr` ASC 기억 같은 세부가 잘 잡혀 있다. 다만 선택지(Choices) 경로가 절반만 구현돼 데이터만으로 소프트락이 나는 상태다. 커버리지: 소스 8개 전부 정독했고, 런타임 경로 확인을 위해 소비처(`AWxPlayerController`·`UWxViewModel_Dialogue`·`UWxAbility_Interact`·`UWxInteractionScannerComponent`)까지 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 1 |

## 결과

### 1. 🔴 Choices 노드에 진입하면 대화가 영원히 닫히지 않는다(소프트락)
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:42-46`
- **범주**: 버그/정확성
- **문제**: `Advance()` 는 마지막 대사에서 `Choices` 가 비어 있지 않으면 아무 것도 하지 않고 return 하며, 진행권을 `Choose()` 에 넘긴다. 그런데 `Choose()` 를 부를 수 있는 쪽이 없다 — 현재 노드의 `Choices` 를 읽는 공개 API 가 `UWxDialogueSessionComponent` 에 하나도 없어(`Public/WxDialogueSessionComponent.h:32-58`) 뷰가 선택지를 그릴 수 없고, 저장소 전체에서 `Choose()` 호출부는 0건이다(뷰모델은 `RequestAdvance` 만 노출: `Source/WxGame/MVVM/WxViewModel_Dialogue.h:45`, `WBP_Dialogue` 도 `AdvanceButton` 하나뿐).
  결과: 디자이너가 DT 행의 `Choices`(`Public/WxDialogueTableRow.h:54`, `EditAnywhere`)를 하나라도 채우면 그 노드에서 세션이 굳는다. `EndDialogue()` 가 안 불리므로 뷰모델 `bFinished` 가 영원히 false → 대화 위젯이 안 닫히고(WBP_Dialogue 의 `InputMode = Menu` 라 게임 입력까지 잠긴다), 폰 ASC 의 `State.Dialogue` 도 남아 상호작용이 계속 차단된다. 폰이 죽어 리스폰하기 전엔 복구가 없다.
  코드가 아니라 데이터만으로 트리거되고, 로그 한 줄 없이 굳는다는 게 특히 나쁘다.
- **제안**: 둘 중 하나를 지금 하고, 나머지를 후속으로 남긴다. (a) 즉시 조치 — `Choices` 가 있는 노드에서 `Advance()` 가 경고 로그 후 `EndDialogue()` 로 폴백하게 해 최소한 갇히지 않게 한다. (b) 본 구현 — 현재 노드의 선택지를 읽는 접근자(+`OnChoicesChanged` 류 델리게이트)를 열어 `Choose()` 경로를 실제로 잇는다.
  어느 쪽이든 세션을 밖에서 끊을 공개 취소 경로(`CancelDialogue()` 등)가 함께 필요하다 — 지금 `EndDialogue()` 는 `Advance`/`Choose` 안에서만 도달 가능하고, `EndPlay`/`UninitializeComponent` 정리도 없다.
- **확신도**: 높음

### 2. 🟡 StartDialogue 재진입 가드가 없어 State.Dialogue 카운트가 새고 위젯이 겹쳐 쌓인다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:84-104`
- **범주**: 설계/구조
- **문제**: `ClientStartDialogue_Implementation` 이 이미 활성 세션인지 보지 않는다. `AddLooseGameplayTag` 는 카운트 증가고 `RemoveLooseGameplayTag` 는 1 감소이므로(엔진 `AbilitySystemComponent.h` 의 `UpdateTagMap` 위임), 두 번 시작되면 종료 뒤에도 카운트 1 이 남아 `State.Dialogue` 가 폰에 영구 부착된다 → `UWxAbility_Interact` 의 `ActivationBlockedTags`(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:37`)와 스캐너 표시 게이트가 그 폰이 죽을 때까지 닫힌 채로 남는다. `OnDialogueStarted` 도 두 번 발행돼 Game 레이어에 대화 위젯이 겹쳐 쌓인다.
  두 번째 시작이 가능한 이유는 차단이 클라 전용이라서다 — 태그는 Client RPC 안에서 소유 클라 폰 ASC 에만 붙는데, `UWxAbility_Interact` 는 `ServerOnly`(같은 파일 `:19`)라 서버 ASC 로 활성 게이트를 판정한다. 서버 ASC 엔 그 태그가 없으므로 서버 쪽 방어선은 사실상 없다. 클라 전송 게이트도 0.1s 주기 스캔 결과에 의존하고(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:36`) `TryInteractSelected`(같은 파일 `:56-66`)는 재검사하지 않으므로, 네트워크 왕복(RTT) 동안 F 를 한 번 더 누르면 서버가 `OnInteracted` 를 다시 호출한다. 스탠드얼론에선 같은 프레임에 위젯이 `InputMode = Menu` 로 올라와 창이 1 프레임 수준이라 사실상 안전하다.
  덤으로 같은 함수의 실패 경로(`:87-91`)는 `Table` 만 null 로 되돌리고 `CurrentRow` 는 직전 테이블의 행을 계속 가리켜, `HasActiveDialogue()` 가 참인데 `Table` 은 없는 어긋난 상태를 남긴다.
- **제안**: 함수 진입부에서 `HasActiveDialogue()` 면 기존 세션을 `EndDialogue()` 로 정리하거나 새 시작을 거부한다. 태그는 `AddLooseGameplayTag` 대신 `SetLooseGameplayTagCount(WxGameplayTags::State_Dialogue, 1)` 로 두면 카운트 누수가 원천 차단된다. 실패 경로에서는 `CurrentRow`/`LineIndex` 도 함께 초기화한다.
- **확신도**: 중간(창이 열리는 건 네트워크 모드뿐이나, 상태 어긋남은 모드 무관)

### 3. 🟡 데이터 오류가 전부 무음으로 사라진다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:14-23`, `:106-117`
- **범주**: 버그/정확성
- **문제**: 실패 경로가 전부 로그 없이 조용히 반환한다 — `StartDialogue` 의 두 조기 반환(대화 정의 없음 / `StartRow` 미지정·테이블 없음), `EnterRow` 실패(행 이름 오타, `Lines` 가 빈 행). 특히 `Advance()` 가 `EnterRow(NextRow)` 실패를 정상 종료와 같은 경로로 처리하므로(`:48-52`), `NextRow` 오타는 "대화가 이유 없이 중간에 끊김"으로만 나타나 원인을 좁힐 단서가 없다. `FindRow` 의 ContextString(`TEXT("WxDialogueSession")`)이 남기는 경고는 행이 아예 없을 때만 뜨고, `Lines` 가 빈 행과 `StartRow` 미지정은 아무 흔적도 남기지 않는다. 모듈은 `LogWxDialogue` 를 선언·정의해 두고(`Public/WxDialogueModule.h:8`, `Private/WxDialogueModule.cpp:6`) 한 번도 쓰지 않는다.
- **제안**: 위 지점들에 `UE_LOG(LogWxDialogue, Warning, ...)` 로 테이블명·행 이름을 남긴다. 디자이너가 DT 를 직접 편집하는 시스템이라 진단 비용 대비 효과가 크다.
- **확신도**: 높음

### 4. 🟢 CurrentRow 가 DataTable 행 메모리를 세션 내내 원시 포인터로 붙든다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:82`
- **범주**: 성능/안전
- **문제**: `Table` 이 `UDataTable` 객체는 붙잡지만 행 메모리는 `UDataTable::EmptyTable()`(리임포트·에디터 편집·`OnPostDataImport`)에서 해제된다. PIE 중 디자이너가 대화 테이블을 건드리면 다음 `Advance()` 가 해제된 메모리를 읽는다. 헤더 주석이 "세션 중에만 유효"라고 못 박아 인지된 제약으로 보이나, 세션 수명이 수 초~수십 초라 창이 짧지 않다.
- **제안**: `CurrentRow` 대신 `CurrentRowName`(FName)만 들고 접근 시점마다 `Table->FindRow` 하면 위험이 사라진다. 조회는 대사를 넘길 때 한 번뿐이라 비용이 없다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxNpc.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`
- **참고로 확인한 모듈 밖 파일**(발견 근거 검증용, 리뷰 대상 아님): `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`
- **규칙 준수 확인(위반 없음)**: 의존성은 `Core`/`CoreUObject`/`Engine`/`GameplayAbilities`/`GameplayTags`/`WxCore` 뿐 — 타 Wx 플러그인 참조 없음. 전 파일 첫 줄 저작권 표기 있음. 전 타입 `Wx` prefix 준수. `BlueprintCallable` 사용 0건. 람다 0건. 델리게이트 콜백 함수 자체가 모듈 내에 없음(발행만 한다). `FWxDialogueModule::StartupModule/ShutdownModule` 의 `Super::` 미호출은 프로젝트 전 플러그인이 동일한 빈 구현이라 확립된 관례로 보아 발견에서 제외했다.
- **미검토 / 한계**: `Content/` 의 `DT_Dialogue` 데이터테이블 실제 행 내용(uasset 이라 `Choices` 가 이미 채워져 있는지는 확인하지 못했다 — 발견 1 의 현재 발현 여부가 여기 달려 있다). `BP_Npc` 인스턴스 디폴트값과 `WBP_Dialogue` 는 스냅샷 수준으로만 확인했다.

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 8파일 — `/module-review`로 갱신*
