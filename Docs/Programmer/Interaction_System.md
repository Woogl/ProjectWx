# 상호작용 시스템 — 등록과 실행

월드의 상호작용 대상이 어떻게 후보로 잡혀 HUD에 노출되고(등록·감지), 입력으로 어떻게 효과까지 실행되는지(트리거→기믹/지급)의 전 경로. 모듈 경계(WxCore / WxWorld / WxGame / WxUI / WxInventory)를 넘어 추적한다.

---

## 한 문장 요약

> PlayerController의 `UWxInteractionScannerComponent`가 소유 클라에서 주변 상호작용 메시를 주기 스캔해 로컬 목록·선택을 소유하고(감지=로컬 어포던스, 비복제), 입력 시 선택 메시를 `ServerInteract` RPC로 서버에 보낸다. 서버가 그 선택을 실어 `Event.Interact`를 폰 ASC로 송출하면 **ServerOnly** `UWxAbility_Interact`가 권위에서 발동해 사거리·활성 검증 후 `IWxInteractable::OnInteracted`를 fire한다.

이 시스템을 가르는 세 축:

- **감지 vs 실행** — 감지(스캔·목록·선택·하이라이트)는 **로컬 전용**이고 어디에도 복제되지 않는다 / 실행(검증→효과)은 **서버 권위**이며 `OnInteracted`는 서버에서만 fire된다(클라 비주얼은 복제 상태로 수렴).
- **영역 = 메시 그 자체** — 상호작용 영역을 대표하는 별도 컴포넌트는 없다. 대상이 `GetActiveInteractionMeshes`로 지금 켜져 있는 영역 메시를 직접 답하고, 그 목록 하나가 표식(어느 메시가 영역인가)과 활성(지금 켜져 있는가)을 겸한다. 콜리전은 전혀 관여하지 않는다. 무슨 일이 일어나고 뭐라고 표시되는지도 같은 인터페이스가 제공하며, 외곽선 강조는 선택을 소유한 스캐너가 단독으로 쓴다.
- **계약의 위치** — 계약 인터페이스 `IWxInteractable`가 `WxCore`에 있다. 소비 도메인(`WxInventory` 픽업 등)은 `WxWorld`를 전혀 보지 않고 인터페이스를 구현하는 것만으로 상호작용 대상이 된다.

---

## 전체 그림

```mermaid
flowchart TD
    subgraph 감지["감지 — 로컬 전용 (소유 클라/리슨 호스트)"]
        Timer["스캐너 스캔 타이머<br/>(PlayerController 컴포넌트)"] --> Scan["액터 순회 → IWxInteractable<br/>GetActiveInteractionMeshes"]
        Scan --> Sort["반경 필터 = 영역, 거리순 정렬"]
        Sort --> Reg["Scanner.UpdateInRange()"]
        Reg -->|"OnListChanged / OnSelectionChanged"| VM["VM_InteractionList"]
        VM --> HUD["WBP_InteractionList (프롬프트·선택 표시)"]
        Reg -->|"ApplyHighlight"| Outline["선택 대상 메시 외곽선"]
    end
    subgraph 실행["실행 — 서버 권위"]
        Input["위젯 Enhanced Input → VM.RequestInteract()"] --> Try["Scanner.TryInteractSelected()"]
        Try --> RPC["ServerInteract(Selected) RPC"]
        RPC --> Send["서버: SendGameplayEventToActor<br/>(Event.Interact, OptionalObject=Selected)"]
        Send --> Ability["Ability_Interact 활성화 (ServerOnly)<br/>사거리 검증"]
        Ability --> Fire["IWxInteractable::OnInteracted(Avatar, 메시)"]
        Fire --> Handler{"소유 액터 핸들러"}
        Handler -->|"기믹"| Commit["CommitGimmickState (권위) → State 복제 → ST"]
        Handler -->|"아이템 픽업"| Grant["인벤토리 지급 (권위) → Destroy"]
    end
```

---

## 등록·감지 — "등록"은 폴링 수집이다

핵심 반전: 대상은 자신을 어디에도 **등록하지 않는다.** 등록의 실체는 (1) 대상이 인터페이스로 답하는 활성 영역 목록과 (2) 플레이어가 그것을 주기 폴링해 로컬 목록에 채우는 것이다.

1. **식별** — 대상이 `GetActiveInteractionMeshes`로 자기 영역 메시를 담아 준다. 메시는 아무 이벤트도 쏘지 않고 지목되기만 하는 수동 표적이다. 한 액터에 영역이 여럿이면(엘리베이터 등) 여럿 담으면 되고, 각각 독립된 후보로 잡힌다.
2. **대상 자격은 콜리전 무관, 사거리만 콜리전 기준** — 누가 영역인가는 인터페이스가 정하므로 프리셋·응답이 무엇이든 상관없다. 다만 사거리는 콜리전 형상으로 재므로(`OverlapComponent` — 채널이 아니라 바디에 직접 던지는 테스트라 응답은 보지 않는다) 영역 메시엔 쿼리 콜리전이 켜져 있어야 하고, 스켈레탈이면 피직스 애셋이 필요하다.
3. **수집 주체** — `UWxInteractionScannerComponent`가 `BeginPlay`에서 월드 타이머매니저에 `ScanAndPush`를 건다(`ScanInterval` 기본 0.1초). **소유 클라(리슨호스트 포함)에서만** 설정한다 — 데디 서버 PC는 스캔하지 않는다.
4. **스캔** — `ScanAndPush`는 `TActorIterator`로 로드된 액터를 훑어 `IWxInteractable` 구현체만 남기고(구현 액터는 소수라 이 캐스트가 사실상의 필터다), 각 활성 영역이 폰에서 `ScanRadius` 안인지 `IWxInteractable::IsMeshInRange`로 재어 후보로 모은 뒤 거리순 정렬해 `UpdateInRange`한다. 상호작용 어빌리티의 `CanActivateAbility` 실패(예: `State.Dead`·`State.Finisher`) 시엔 빈 배열을 넣어 목록·선택·하이라이트를 즉시 정리한다 — 차단 조건의 단일 소스는 어빌리티이므로 컴포넌트가 상태 태그를 하드코딩하지 않는다.
5. **생명주기 토글** — 활성/비활성은 영역이 그 목록에 있느냐 없느냐가 전부다. 멤버십 자체가 상태이므로 따로 저장할 bool이 없다. 빠진 메시는 다음 스캔에서 탈락하고 그때 외곽선도 꺼진다. 기믹은 베이스(`AWxGimmick`)가 활성 영역 집합을 소유하고 StateTree의 `Enable Interaction` 노드가 상태별로 넣고 뺀다(예: 문이 열리면 콘솔 인터랙션 비활성). 적 처형은 목록을 늘 열어 두고 자격 판정(`CanBeInteractedBy`)에 맡긴다.

활성 상태는 **복제하지 않는다.** 각 머신이 로컬로 같은 값에 수렴하기 때문이다 — 기믹은 복제 State로 StateTree가 서버·클라 양쪽에서 토글을 구동하고, 적 처형 자격은 복제되는 판정 입력(HP·`State.Groggy`·`State.InCombat`·트랜스폼·`State.Finisher`)으로 각 머신이 직접 평가한다.

`UWxInteractionScannerComponent`는 `AWxPlayerController`에 붙는다 — 폰 리스폰에도 생존하고, 소유 클라 연결로 net-owned라 `ServerInteract` RPC를 직접 들 수 있으며, 타 클라에 복제되지 않아 로컬리티가 좋다. `UpdateInRange`는 **기존 순서를 보존**하고 신규만 뒤에 append, 이탈은 제거하며, 멤버십이 실제로 바뀐 경우에만 발화한다(거리 변동에 목록이 흔들리지 않게). 갱신 전 선택 메시를 포인터로 캐시해 순서가 바뀌어도 같은 대상으로 선택을 잇는다. 선택(`SelectedIndex`)을 이 컴포넌트가 **소유**하고, 선택된 메시에만 외곽선을 켠다(`ApplyHighlight`). 외곽선(Custom Depth/Stencil)을 쓰는 주체는 이 하나뿐이다.

---

## 후보 목록 → UI

스캐너(`WxWorld`)와 항목 뷰모델(`WxUI`)은 서로를 참조할 수 없으므로 `WxGame`의 리스트 VM·리졸버가 다리를 놓는다.

- **리졸버** `UWxViewModelResolver_InteractionList`(WBP의 View Bindings에서 `Creation Type = Resolver`)가 `CreateInstance`에서 PlayerController의 스캐너를 찾아 `UWxViewModel_InteractionList`를 생성하고, 스캐너의 `OnListChanged`/`OnSelectionChanged`를 VM의 `HandleListChanged`/`HandleSelectionChanged`에 연결한 뒤 현재 목록·선택으로 시드한다.
- **VM** `UWxViewModel_InteractionList`(`WxGame`)는 프롬프트 목록을 항목 VM(`UWxViewModel_Interaction`(`WxUI`): `Prompt`+`bSelected`) 배열로 재구성하고 선택 인덱스를 표시한다.
- **입력** — HUD 리스트 위젯이 Enhanced Input으로 받아 VM의 `RequestInteract()`/`RequestCycle(Delta)`를 호출하고, VM이 스캐너의 `TryInteractSelected`/`CycleSelection`으로 넘긴다. 선택의 단일 소유자는 스캐너다.
- 프롬프트 문자열은 각 대상의 `IWxInteractable::GetInteractionPrompt(Source)`에서 온다(스캐너가 스캔 때 pull). `Source`는 문구를 물어보는 대상 메시라, 응답과 마찬가지로 영역별로 다른 문구를 낼 수 있다.
- 기믹의 프롬프트는 두 층에서 author 한다. **영역별 고정 문구**는 BP 디폴트에 두고(`AWxGimmick::GetDefaultInteractionPrompt`를 override 해 `Source`로 분기 — `AWxElevator`가 이 방식), **상태별 문구**는 `Enable Interaction` 태스크의 `Prompt` 필드가 그 대상 메시에 로컬로 세팅한다. `AWxGimmick::GetInteractionPrompt`가 우선순위(상태별 → 영역별 → `InteractionPrompt` 기본값)를 소유하므로 자식은 폴백만 신경 쓴다.

---

## 실행 — 입력에서 효과까지 (서버 권위)

선택은 복제하지 않으므로 실행 시점에 **로컬 선택을 원자적으로 전송**한다. `TryInteractSelected`가 현재 선택 컴포넌트를 읽어 `ServerInteract(Selected)` RPC로 보내면(로컬 동기 읽기라 "사이클→즉시입력" 순서가 보장된다), 서버가 그 선택을 `FGameplayEventData.OptionalObject`에 실어 `Event.Interact`를 폰 ASC로 송출한다. `UWxAbility_Interact`(`NetExecutionPolicy = ServerOnly`, `AbilityTriggers = Event.Interact`, `ActivationBlockedTags = State.Dead·State.Finisher`)가 그 이벤트로 권위에서 발동한다.

`ServerOnly`라 클라 인스턴스가 없다 — 코스메틱 예측이 없고(상호작용 연출은 대상의 StateTree가 담당), 실행은 서버에서만 일어난다. 선택 페이로드를 나르는 통로가 자체 RPC이므로 `LocalPredicted`의 `ServerTryActivateAbilityWithEventData` 통로가 필요 없다.

`ExecuteInteract`는 두 가지를 서버에서 다시 확인한다 — 선택 메시가 대상의 `GetActiveInteractionMeshes` 목록에 들어 있는지(활성), 그리고 아바타가 그 영역에서 `ScanRadius` 이내인지(사거리). 변조 클라가 비활성이거나 먼 메시를 보내도 여기서 걸린다. 사거리 식은 `IWxInteractable::IsMeshInRange` 하나를 클라 스캔과 공유하되, 상수는 감지(스캐너)와 검증(어빌리티)이 각자 보유하며 값을 일치시켜야 정합한다.

선택 메시는 실행 경로 전체에서 읽기만 하므로 `const`로 흐른다 — `OnInteracted`의 `Source`도 `const UActorComponent*`라 `const_cast`가 필요 없다.

```mermaid
sequenceDiagram
    autonumber
    participant W as HUD 리스트 위젯 (클라)
    participant VM as VM_InteractionList
    participant R as Scanner (PC 컴포넌트)
    participant SV as 서버
    participant AB as Ability_Interact (ServerOnly)
    participant IC as 대상 메시
    participant H as 소유 액터 핸들러
    W->>VM: Enhanced Input → RequestInteract()
    VM->>R: TryInteractSelected()
    R->>SV: ServerInteract(Selected) RPC
    SV->>AB: SendGameplayEventToActor(Event.Interact, OptionalObject=Selected)
    AB->>IC: 사거리·활성(활성 목록 포함 여부) 검증
    IC->>H: IWxInteractable::OnInteracted(Instigator, Source) [서버 전용]
```

소유자의 `IWxInteractable::OnInteracted`는 서버 권위에서만 호출된다(클라 비주얼은 각 대상의 복제 상태로 수렴). 두 번째 인자 `Source`는 이번 상호작용이 일어난 대상 메시로, 한 액터에 영역이 여럿일 때 영역을 가르는 데 쓴다(단일 영역이면 무시). 읽기 전용이라 `const UActorComponent*`다.

### 효과 — 대상별 핸들러

- **기믹** (`AWxGimmick` 자식, 예: `AWxDoor`): 자식이 C++ 생성자에서 자기 콘솔/메시를 베이스의 활성 영역 집합에 담고, `OnInteracted`를 override 해 `CommitGimmickState(NewState)`로 State를 확정한다(서버 권위 호출이라 핸들러 내 별도 가드 불필요). State는 복제+SaveGame이며, `OnRep_GimmickState`(권위는 Commit이 직접 호출)가 그 태그를 StateTree 이벤트로 발행해 비주얼·인터랙션 토글·사이드이펙트를 구동한다. 클라 핸들러는 노옵 — 복제된 State가 ST 진입을 구동하므로 클라가 직접 State를 쓰지 않는다(기믹 전이의 서버 권위 규칙).
- **다중 영역** (`AWxElevator`): 플랫폼·콜콘솔A·콜콘솔B 세 메시를 각각 담고, `OnInteracted`가 `Source`를 자기 메시 멤버와 비교해 분기한다. `GetDefaultInteractionPrompt`도 같은 분기로 영역별 프롬프트 필드(BP 디폴트에서 author)를 고른다.
- **아이템 픽업** (`AWxItemPickup`, WxInventory): 액터 자신이 `IWxInteractable`를 구현해 자기 메시를 상시 활성 영역으로 답한다 — 계약이 `WxCore`에 있어 `WxWorld` 참조 없이 가능하므로 BP 작업이 필요 없다. `OnInteracted`에서 `UWxInventoryManagerComponent::FindInventory(Interactor)`로 인벤토리에 지급 후 `Destroy` 하고, `GetInteractionPrompt`가 `"[F] {DisplayName}"` 프롬프트를 반환한다(BP 배선·자동 바인딩 불필요).
- **적 처형** (`AWxEnemyCharacter`, WxGame): 캐릭터 메시가 곧 영역이며 목록은 늘 열려 있다. 실제 노출 여부는 `CanBeInteractedBy`가 주체별로 판정한다(그로기=앞잡 / 미인지·후방=뒤잡). 판정은 `GetEligibleFinisherEventTag` 한 곳에 모여 있어 노출과 발동 검증이 같은 규칙을 쓴다. 연출 중에는 `UWxAbility_Finisher`가 대상 ASC에 `State.Finisher`를 복제 루즈 태그로 걸어 두므로, 재노출도 다른 플레이어의 중복 발동도 함께 막힌다.

`AWxGimmick`의 다른 자식들(`WxTreasureChest`, `WxAlarmConsole`, `WxSpawnConsole`, `WxCutsceneTrigger`)도 같은 `OnInteracted`→`CommitGimmickState` 패턴을 따른다(`WxCutsceneTrigger`는 일시 상태라 복원 시 Idle 리셋).

---

## 아키텍처 제약이 강제한 설계

- **모든 플러그인은 WxCore만 참조 가능, 도메인↔도메인 의존 금지** → 계약 `IWxInteractable`(영역 + 응답 + 프롬프트)를 `WxCore`에 둔다. `WxWorld`에는 스캐너만 남으므로, `WxInventory` 픽업은 `WxWorld`를 모른 채 인터페이스를 구현하는 것만으로 대상이 된다.
- **WxUI는 WxWorld(스캐너)를 못 본다** → 양쪽에 의존할 수 있는 `WxGame`이 리스트 VM과 리졸버를 갖고 델리게이트를 연결한다(통합 모듈이 다리, 의존 방향 보존).
- **감지는 로컬, 실행은 권위** → 선택은 복제하지 않고 실행 시점에 `ServerInteract` RPC로 원자 전송한다. 스캐너는 소유 클라에서만 구동하며 상태를 복제하지 않는다.

---

## 주의할 점

- **`OnInteracted`는 서버 권위에서만 fire** — 핸들러는 권위 로직을 그대로 수행한다. 클라 비주얼은 각 대상의 복제 상태(기믹 State, 픽업 Destroy 등)로 수렴한다.
- **영역 메시엔 쿼리 콜리전이 필요하다** — 사거리를 콜리전 형상으로 재므로, 콜리전을 끈 메시나 심플 콜리전이 없는 스태틱 메시는 사거리 판정이 항상 실패해 조용히 프롬프트가 뜨지 않는다. 응답·프로파일은 무엇이든 상관없고 켜져 있기만 하면 된다.
- **비활성 시 외곽선은 다음 스캔에 꺼진다** — 강조를 쓰는 주체가 스캐너 하나뿐이라, 선택 중인 영역이 꺼지면 최대 `ScanInterval`만큼 외곽선이 남는다.
- **선택 페이로드의 net-addressable 요구** — `ServerInteract`가 컴포넌트 포인터를 PackageMap으로 직렬화하므로, 동적 스폰 액터(픽업·적)의 컴포넌트도 복제돼야 한다(`SetIsReplicatedByDefault(true)`). 복제 프로퍼티는 없지만 이 이유로 복제 설정 자체는 유지한다. 막 스폰돼 아직 복제 안 된 대상은 서버에서 null로 도착해 무동작할 수 있다(권위 게이트라 안전).
- **액터 순회 비용** — 브로드페이즈가 엔진 공간 분할이 아니라 `TActorIterator` 순회다. 소유 클라 1인이 `ScanInterval`마다 로드된 액터를 훑고 인터페이스 캐스트로 거르므로 현 규모에선 무시할 수준이지만, 대상 종류가 크게 늘면 등록 목록 도입을 검토한다(계약은 그대로 재사용된다).
- **책임 경계** — 컴포넌트는 프롬프트도 영역도 소유하지 않는다(대상이 인터페이스로 제공). 선택 소유는 스캐너, 표시는 VM, 입력 수신은 HUD 위젯, 실행 권위는 서버로 분리돼 있다.

---

### 참조 코드

| 타입 | 모듈 | 역할 |
| --- | --- | --- |
| `IWxInteractable` | `WxCore` | 대상 계약 인터페이스(`GetActiveInteractionMeshes` 활성 영역 / `OnInteracted` 응답 / `GetInteractionPrompt` 문구, 뒤 둘은 `Source` 메시로 영역을 가름). 대상 액터가 직접 구현, 소비 도메인이 WxWorld 없이 대상을 만드는 접점. `Find`가 영역 메시에서 구현체를 되찾는 유일한 조회 지점이다 |
| `Event_Interact` | `WxCore` | 상호작용 발동 GameplayEvent 태그. 서버가 선택 대상을 실어 송출, 어빌리티가 트리거 |
| `UWxInteractionScannerComponent` | `WxWorld` | PlayerController 컴포넌트. 주기 스캔·인-레인지 목록·선택 소유. `TryInteractSelected`/`CycleSelection`/`ServerInteract` |
| `AWxGimmick` / `AWxDoor` / `AWxElevator` | `WxWorld` | 상호작용 대상 구현(기믹). `OnInteracted`→`CommitGimmickState`(권위)→복제 State→StateTree |
| `UWxAbility_Interact` | `WxGame` | 권위 실행 전용(`ServerOnly`). 페이로드 선택 메시의 사거리·활성 검증 후 소유 액터 `OnInteracted` 호출 |
| `UWxViewModel_InteractionList` / `UWxViewModelResolver_InteractionList` | `WxGame` | 스캐너(WxWorld)↔항목 VM(WxUI) 연결·시드, 위젯 입력을 스캐너로 중계 |
| `UWxViewModel_Interaction` | `WxUI` | HUD 항목 표시 전용 VM(`Prompt`+`bSelected`) |
| `AWxItemPickup` | `WxInventory` | 비-기믹 대상 구현(`IWxInteractable`). 자기 메시를 상시 활성 영역으로 답하고, `OnInteracted`→인벤토리 지급 |
| `AWxEnemyCharacter` | `WxGame` | 처형 대상 구현. 자격을 `CanBeInteractedBy`가 전 머신에서 로컬 평가, 연출 중엔 `State.Finisher`로 차단 |
