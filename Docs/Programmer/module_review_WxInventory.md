# WxInventory — 코드 리뷰

> 모듈 경계와 코딩 규칙은 여전히 흠이 없다(WxCore 외 Wx 의존 0, 저작권 헤더 22/22, 람다·FORCEINLINE 0, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward` 한 곳뿐이며 BP Function Library 라 정당). 남은 문제는 전부 복제·통지 계층에 몰려 있고, 대부분 이전 리뷰에서 지적된 뒤 그대로다 — 다만 이번에 소비자(`UWxViewModel_Inventory`)를 교차 확인해 "이론상 위험"으로 남겨뒀던 델타 계약 문제가 실제로 화면에 틀린 수량을 띄운다는 것이 확인됐다. 이번 리뷰는 `README.md`·`.uplugin`·`*.Build.cs`·전체 Public 헤더를 읽고 `WxInventoryComponent.cpp`, `WxItemInstance.cpp`, `WxItemPickup.cpp`, `WxRewardLibrary.cpp`, `WxEquipmentComponent.cpp`, StateTree 태스크 2종을 정독했으며, 계약 확인용으로 `Source/WxGame` 의 소비자 코드를 교차 조회했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 스택 변경 통지가 슬롯마다 쪼개져 발행돼 획득 토스트가 부분 수량만 표시한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:304`, `:320`, `:413`, `:619-628` / 소비자 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104-122`
- **범주**: 버그/정확성
- **문제**: 같은 델리게이트가 경로마다 다른 입자도로 발행된다 — 추가 경로는 머지·분할 슬롯 하나마다 1회(304·320행), 소비 경로는 배치 전체에 1회(413행, `-NumToConsume`), 클라이언트 복제 경로는 엔트리 하나마다 1회(83-90·101-112행). 이전 리뷰는 "`NewCount` 만 읽는 현재 뷰모델은 맞게 동작한다"고 판단했으나, 실제 구독자는 `Delta` 도 쓴다: `UWxViewModel_Inventory::HandleStackChanged` 는 `Delta > 0` 일 때마다 획득 연출용 `LastAcquiredItem` VM 을 새로 만들고 `AcquiredCount = Delta` 를 넣는다(110-119행). 따라서 스택 상한을 넘겨 분할되는 지급이나 비스택 아이템 지급에서 마지막 조각의 델타만 남는다 — 예컨대 `MaxStack` 없는(=1) 아이템을 보상으로 5개 주면 `Delta == 1` 짜리 통지가 5번 나가 토스트에 "x1" 이 뜨고, 97/99 인 슬롯에 5개를 더하면 "x2" 뒤 "x3" 이 떠 최종 표시가 x3 이 된다. 서버·스탠드얼론에서 그대로 재현되며 네트워크에서는 발행 횟수가 또 달라진다. 여기에 `NotifyStackChangedFromList` 는 발행할 때마다 626행에서 `GetTotalItemCountByDefinition` 으로 엔트리 전체를 훑으므로 대량 지급 시 O(N·Entries) 재스캔과 구독자 콜백 증폭이 겹친다.
- **제안**: 발행 단위를 "변경 배치당 1회"로 통일한다 — `AddItemDefinition` 은 누적 델타를 모아 함수 끝에서 한 번만 발행하고(소비 경로가 이미 그 형태다), 복제 콜백도 배치 단위로 묶는다. 슬롯 단위 정보는 `OnInventorySlotChanged` 가 이미 담당한다. 배치화하면 전체 재스캔도 지급당 1회로 줄어든다.
- **확신도**: 높음

### 2. 🟡 FastArray 수신 콜백이 널 검사보다 먼저 `LastObservedCount` 를 갱신해 클라이언트 획득 델타가 영구 유실된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:83`, `:85`, `:88`, `:104-107`, `:621`
- **범주**: 버그/정확성
- **문제**: `PostReplicatedAdd` 는 83행에서 `Entry.LastObservedCount = Entry.StackCount;` 를 무조건 수행한 뒤 85행에서야 `Entry.Instance` 널 검사를 한다. 클라이언트가 엔트리를 받는 시점에 서브오브젝트 참조가 아직 NetGUID 미해결이면 통지는 건너뛰는데 관찰값만 최신으로 올라간다. 포인터가 나중에 해결돼도 FastArray 는 그 항목을 다시 dirty 로 보지 않고, `PostReplicatedChange` 가 오더라도 `Delta == 0` 이라 107행에서 걸러진다. **두 번째 경로도 같은 결과를 낸다**: 인스턴스는 해결됐지만 그 위의 `ItemDef` 가 아직 도착하지 않으면 88행이 `nullptr` 을 넘기고 `NotifyStackChangedFromList` 가 621행에서 조용히 반환한다 — 이때도 `LastObservedCount` 는 이미 최신이라 복구 기회가 없다(`HandleItemDefReplicated` 는 의도적으로 델타를 만들지 않고 "다시 읽어라" 신호만 보낸다). 결과적으로 그 슬롯의 획득 델타 1회가 사라지고, 발견 1 에서 확인한 `LastAcquiredItem` 획득 연출이 해당 아이템에 대해 아예 뜨지 않는다. 목록 **표시** 는 `PostReplicatedReceive` → `OnInventoryContentsChanged`(120-127행)가 뒤늦게 따라잡는다. `PostReplicatedChange`(104-105행)도 같은 형태로 델타를 삼킨다.
- **제안**: `LastObservedCount` 갱신을 실제로 통지를 발행한 경로 안(널 검사 + `ItemDef` 유효 확인 이후)으로 옮긴다. 그러면 미해결 슬롯은 다음 변경 때 누적 델타로 복구된다.
- **확신도**: 중간

### 3. 🟡 통지·확장점 호출 너머까지 `Entries` 배열 참조를 붙들고 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:136-151`, `:286-310`
- **범주**: 버그/정확성
- **문제**: 두 지점이 같은 형태다. (a) `FWxInventoryList::AddEntry` 는 136행 `Entries.AddDefaulted_GetRef()` 로 얻은 참조 `NewEntry` 를 들고 142-148행에서 가상 확장점 `Fragment->OnInstanceCreated(NewEntry.Instance)` 를 호출한 뒤 150행 `MarkItemDirty(NewEntry)`·151행 `return NewEntry.Instance` 를 이어간다. `OnInstanceCreated` 는 README 가 공식 확장점으로 광고하는 가상 함수인데(`Public/Items/WxItemFragment.h:40`), 그 안에서 같은 인벤토리에 아이템이 추가되면 `Entries` 재할당으로 `NewEntry` 가 댕글링된다. (b) `AddItemDefinition` 의 머지 루프는 286행에서 `const TArray<FWxInventoryEntry>& Entries` 를 잡고 루프 조건(287행)과 인덱싱(289·294·299행)에 계속 쓰면서, 루프 **안**의 303-304행에서 외부 구독자에게 브로드캐스트한다 — 구독자가 같은 인벤토리를 건드리면 참조와 `Entries.Num()` 이 동시에 무효해진다. (b)는 임의의 구독자가 트리거할 수 있어 (a)보다 도달 경로가 넓다(현재 구독자인 뷰모델들은 읽기만 해서 발현되지 않는다).
- **제안**: (a) `const int32 NewIndex = Entries.AddDefaulted();` 로 인덱스를 잡고 프래그먼트 호출 이후 `Entries[NewIndex]` 로 다시 접근한다. (b) 발견 1 의 배치 발행으로 옮기면 루프 안 브로드캐스트가 사라져 함께 해소된다. 즉시 고치려면 머지 대상 인덱스·수량을 먼저 수집한 뒤 루프 밖에서 통지한다.
- **확신도**: 중간

### 4. 🟡 장비 경로는 여전히 배선만 있고, 그 복제 비용은 전 캐릭터가 낸다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:14`, `:34-57`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h:179`, `:234`, `Source/WxGame/Character/WxCharacterBase.cpp:42`
- **범주**: 설계/구조
- **문제**: `UWxEquipmentComponent::EquipItem` 의 유일한 호출부인 `UWxInventoryComponent::EquipItemByDef` 를 부르는 곳이 프로젝트 전체에 0건이라(헤더 232행이 스스로 그렇게 적고 있다) `EquippedItemDef` 는 항상 널이고 `ApplyEquipEffects`/`RemoveActiveEquipEffects` 는 도달 불가다. 그런데 `WxCharacterBase.cpp:42` 가 모든 캐릭터(플레이어·적·미니언)에 이 컴포넌트를 기본 서브오브젝트로 만들고 컴포넌트는 `SetIsReplicatedByDefault(true)`(14행)라, 액터마다 쓰이지 않는 복제 등록 슬롯을 하나씩 계속 소모한다. `UWxInventoryComponent::RemoveItemInstance`(179행)도 호출부 0건이다.
- **제안**: 장비 기능을 곧 쓸 계획이 없으면 `UWxEquipmentComponent`·`EquipItemByDef`·`RemoveItemInstance`·`UWxItemFragment_Equippable` 을 걷어낸다. 유지한다면 최소한 `WxCharacterBase` 의 무조건 생성만이라도 거둔다(장비를 실제로 쓰는 캐릭터에만 부착).
- **확신도**: 높음

### 5. 🟢 `MaxStack` 이 0 이하면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:279`, `:313-331`
- **범주**: 성능/안전
- **문제**: 315행 `ChunkCount = FMath::Min(MaxStack, Remaining)` 이 0 이면 326행의 `Remaining -= ChunkCount` 가 진전을 못 만들어, while 루프가 매 회전마다 새 엔트리와 `UWxItemInstance` 를 만들며 영원히 돈다(행 + OOM). `UWxItemFragment_Stackable::MaxStack` 의 `ClampMin = "1"`(`Public/Items/WxItemFragment.h:109`)은 디테일 패널 입력만 막지, 이전에 저장된 값이나 툴셋·스크립트로 직접 쓰인 값은 막지 못한다. 이전 리뷰에서 지적된 뒤 그대로다.
- **제안**: 279행을 `const int32 MaxStack = FMath::Max(1, Stackable ? Stackable->MaxStack : 1);` 로 한 번 방어한다.
- **확신도**: 중간

### 6. 🟢 `GrantReward` 가 `World` 만 검사 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:43`, `:75`
- **범주**: 성능/안전
- **문제**: `SourceActor`·`RewardRow`·`Row`·`ItemDef`·`ItemActorClass`·`SpawnedPickup` 을 모두 방어적으로 검사하는 함수에서 `UWorld* World = SourceActor->GetWorld();`(43행)만 검사 없이 75행에서 `World->SpawnActorDeferred` 로 넘어간다. BP 에 노출된 유일한 진입점이라 CDO·정리 중인 액터가 들어올 여지가 있고, 다른 방어와 일관되지도 않다.
- **제안**: 43행 뒤에 `if (!World) { return; }` 를 붙인다.
- **확신도**: 중간

### 7. 🟢 같은 클래스 안에서 OnRep 함수 명명이 갈린다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h:73`, `:76`, `:79`, `:82`
- **범주**: 규칙 위반
- **문제**: `UWxItemInstance` 의 두 `ReplicatedUsing` 콜백이 각각 `HandleItemDefReplicated`(73행)와 `OnRep_CurrentCharges`(76행)로 서로 다른 규약을 쓴다. CLAUDE.md 규칙 4 의 `Handle` 접두는 델리게이트 콜백 대상이라 OnRep 은 엄밀히 위반은 아니지만, 한 클래스 안에서 두 규약이 섞여 있어 `OnRep_` 으로 검색하는 다음 독자가 절반만 찾는다. 모듈의 다른 OnRep(`AWxItemPickup::OnRep_ItemDef`)은 `OnRep_` 쪽이다.
- **제안**: 모듈 다수파인 `OnRep_ItemDef` 로 통일한다(`HandleItemDefReplicated` → `OnRep_ItemDef`).
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`
- **훑은 파일**: `Plugins/WxInventory/README.md`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `.../Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `.../Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `.../Private/Items/WxItemDefinition.cpp`, `.../Private/Items/WxItemFragment.cpp`, `.../Private/Items/WxRewardTableRow.cpp`, `.../Private/WxInventoryModule.cpp`, 나머지 Public 헤더 전부. 계약 교차 확인으로 `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`
- **미검토 / 한계**:
  - 이전 리뷰의 "GameFeatureAction 이 `Super::` 를 부르지 않는다" 항목은 `WxGameFeatureAction_AddInventoryItems` 가 Experience/ModularGameplay 제거(`97eb35f9`)와 함께 삭제되어 해소됐다. 시작 아이템은 지금 `UWxInventoryComponent::BeginPlay` 가 권한 측에서 직접 지급하며(`WxInventoryComponent.cpp:225-236`), 등록 순서(`RegisterReplicatedInstance` ↔ `ReadyForReplication`)는 어느 쪽이 먼저 와도 서브오브젝트 등록이 성립하는 것을 확인했다.
  - StateTree 태스크 2종이 대상 인벤토리를 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 고정하는 점(`WxStateTreeTask_GiveRewards.cpp:40`, `WxStateTreeTask_RefillItemCharges.cpp:36`)은 이전 리뷰 판단대로 모듈 고유 결함이 아니라 프로젝트 전반의 싱글플레이 전제라 발견으로 올리지 않았다. 멀티 정책이 정해지는 시점에 WxQuest·WxDialogue 와 함께 봐야 한다.
  - "Pawn → PlayerController → `FindComponentByClass<UWxInventoryComponent>`" 관용구가 5곳에 복제돼 있으나(`WxItemPickup.cpp:90`, `WxRewardLibrary.cpp:41`, `WxStateTreeTask_RefillItemCharges.cpp:37`, `WxItemInstance.cpp:86`·`:96`), 공용 정적 헬퍼는 과거에 의도적으로 제거된 것이라 재제안하지 않았다.
  - 서버 권한 API 의 `check(HasAuthority())` 계약(`WxInventoryComponent.cpp:276`, `:360`, `:394`, `:512`, `:576`, `:593`)은 Lyra 식 계약을 그대로 따른 것으로 보아 판단하지 않았다. 유일한 사용 경로(`UWxItemUseComponent::HandleUseItemEvent`)가 앞단에서 권한을 거르는 것은 확인했다.
  - `FWxInventoryList::OwnerComponent` 백포인터가 컴포넌트 템플릿 인스턴싱 시 아키타입 값으로 덮이는지는 엔진 내부 `InitProperties` 순서에 달려 있어 코드만으로 단정할 수 없었다. Lyra 가 동일 패턴을 쓰고 현재 게임이 정상 동작하므로 발견으로 올리지 않았다.
  - 발견 2 는 실제 델타 패킷 순서에 달린 문제라 PIE 리슨/데디 조합 실측 없이 코드 경로 추론에 근거한다. 이 환경에서는 빌드·에디터 실행이 불가능해 컴파일·런타임 검증을 하지 않았다.
  - 아이템 정의·보상 DataTable 등 데이터 에셋의 실제 값(`MaxStack`, `ItemActorClass` 설정 여부 등)과 BP/WBP 내부 구조는 열어보지 않았다.

---
*문서 기준 커밋 `53bc7de6` · 리뷰일 2026-09-08 · 소스 22파일 — `/module-review`로 갱신*
