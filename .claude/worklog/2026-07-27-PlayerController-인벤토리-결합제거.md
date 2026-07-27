# PlayerController 에서 인벤토리 결합 제거

## 계획

### 목표

`AWxPlayerController` 에 남아 있던 인벤토리 지식 5군데를 전부 걷어낸다. 직전 커밋이 인벤토리 부착을 GameMode 주입으로 옮겼지만, 조회 게터·시작 아이템 목록·지급 호출·HUD 게이트·복제 도착 후크가 PC 에 남아 있었다. 시작 아이템은 GameMode 가 소유·지급하고, HUD 게이트는 인벤토리 뷰모델을 관찰형으로 바꿔 없앤다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/.../Inventory/WxInventoryManagerComponent.h/.cpp` | 클래스 차원 도착 신호(`OnAnyInventoryReady`) 추가, `BeginPlay` 에서 발행 | 수정 |
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | `DefaultInventoryItems` 소유, `PostLogin` 에서 지급 | 수정 |
| `Source/WxGame/Controller/WxPlayerController.h/.cpp` | 인벤토리 게터·목록·지급·HUD 게이트·`OnSubobjectCreatedFromReplication` 제거 | 수정 |
| `Source/WxGame/MVVM/WxViewModel_Inventory.h/.cpp` | 관찰형 전환(`StartObserving`), 리졸버 Outer 를 PC 로 | 수정 |
| `Source/WxGame/MVVM/WxViewModel_Item.cpp` | 조회를 `FindInventory` 로, PC 캐스팅 제거 | 수정 |
| `/Game/Framework/BP_PlayerController`, `/Game/Framework/GM_Combat` | 시작 아이템 목록 이전 | 수정(MCP) |

### 접근 방식

- **HUD 게이트의 진짜 원인은 리졸버의 1회성**: `UWxViewModelResolver_Inventory` 가 위젯 생성 시점에 인벤토리를 한 번 찾고 없으면 `nullptr` 을 반환해 바인딩이 빈 채로 굳는다. PC 의 게이트와 복제 후크는 그걸 대신 막아주던 우회였다. 리졸버를 관찰형으로 바꾸면 두 우회가 함께 사라지고, HUD 가 인벤토리 도착 순서에 의존하지 않게 된다.

- **관찰형은 보스 네임플레이트 전례를 따른다**: `UWxViewModel_BossCharacter` 가 "위젯은 상시, 대상은 늦게 등장" 이라는 같은 문제를 이미 그 방식으로 푼다 — 리졸버가 돌려준 인스턴스는 고정하고 내부 상태(`Initialize`/`Deinitialize`)만 갈아끼운다. 인벤토리 VM 은 그 구조를 이미 갖고 있어 관찰 시작점만 붙이면 된다.

- **도착 신호는 클래스 차원 델리게이트**: 관찰자가 컴포넌트보다 먼저 존재할 수 있어 인스턴스 델리게이트로는 잡을 수 없다. 컴포넌트가 `BeginPlay` 에서 발행하면 주입(서버)·복제 도착(클라) 두 경로가 한 지점으로 수렴한다.

- **지급 시점 안전성**: 주입은 PC 의 `PreInitializeComponents` 에서 동기 완료되어 `PostLogin` 보다 앞서고, `ReadyForReplication` 이 기존 엔트리를 back-fill 하므로 그 이전에 지급해도 복제 등록이 누락되지 않는다.

---

## 완료

### 수정한 파일

| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/.../Inventory/WxInventoryManagerComponent.h/.cpp` | `FWxOnInventoryReady` + `static OnAnyInventoryReady`, `BeginPlay` 에서 발행 | 수정 |
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | `DefaultInventoryItems` 소유, `PostLogin` 에서 `FindInventory` → `GrantItems` | 수정 |
| `Source/WxGame/Controller/WxPlayerController.h/.cpp` | 게터·목록·지급·HUD 게이트·`OnSubobjectCreatedFromReplication`·인벤토리 include 전부 제거 | 수정 |
| `Source/WxGame/MVVM/WxViewModel_Inventory.h/.cpp` | `StartObserving`/`HandleInventoryReady`/`StopObserving` 추가, 리졸버가 항상 VM 반환·Outer 를 PC 로 | 수정 |
| `Source/WxGame/MVVM/WxViewModel_Item.cpp` | 조회를 `FindInventory` 로, PC include·캐스팅 제거 | 수정 |

### 구현·결정과 그 이유

- **HUD 게이트를 옮기지 않고 없앴다**: PC 의 게이트와 복제 후크는 리졸버가 1회성이라 생긴 우회였다. 원인 쪽(리졸버)을 고치니 두 우회가 갈 곳 없이 사라졌다 — 결합을 다른 클래스로 이사시키는 대신 결합의 이유 자체를 제거한 셈이다. 덤으로 "인벤토리가 늦게 오면 HUD 가 아예 안 뜬다"는 취약한 순서 의존도 함께 없어졌다.

- **도착 신호를 클래스 차원 정적 델리게이트로**: 관찰자(뷰모델)가 관찰 대상(인벤토리)보다 먼저 존재할 수 있어 인스턴스 델리게이트로는 구독 시점이 안 나온다. 컴포넌트가 `BeginPlay` 에서 발행하면 주입(서버)·복제 도착(클라) 두 경로가 한 지점으로 수렴하고, 구독자는 소유 액터로 자기 것인지 가린다.

- **VM 의 Outer 를 인벤토리에서 PC 로**: 생성 시점에 인벤토리가 없을 수 있어 더 이상 Outer 로 쓸 수 없다. 보스 네임플레이트 VM 과 같은 선택이다.

- **PC 에 남은 결합은 대화 세션 하나**: 인벤토리·스캐너는 전부 빠졌고, `DialogueSession` 만 생성자 서브오브젝트로 남아 있다.

### 계획 대비 달라진 점

계획대로. 다만 `DefaultInventoryItems` 를 PC 에서 지우기 전에 `BP_PlayerController` 의 기존 값을 먼저 읽어뒀어야 했는데 순서가 반대였다(아래 후속 과제).

### 후속 과제

- **`GM_Combat.DefaultInventoryItems` 값 입력 (미완)**: PC 에서 프로퍼티를 먼저 제거하는 바람에 `BP_PlayerController` 에 저장돼 있던 기존 값이 리플렉션으로 읽히지 않는다. 애셋 바이트에서 대상이 `/Game/Item/DA_Potion` 인 것까지는 확인했으나 수량은 확인하지 못했다. 값 입력 후 BP 쪽 잔여 오버라이드를 정리해야 한다.
- **PIE 검증 미완**: 에디터를 띄우지 못해 시작 아이템 지급·HUD 인벤토리 표시·획득 시 갱신을 실제로 확인하지 못했다. 빌드(`Result: Succeeded`, 경고 0)까지만 확인됐다.
- 대화 세션(`DialogueSession`) 결합 제거는 별도 작업. `Client` RPC 가 있어 주입 전환 시 복제 플래그가 필요하다.
