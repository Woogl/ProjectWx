# WxInventory — 코드 리뷰

> 모듈 경계와 코딩 규칙은 여전히 깨끗하고(WxCore 외 Wx 의존 0, 저작권 헤더 24/24, 람다·FORCEINLINE 0, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward` 한 곳뿐이며 정당), 복제 통지 순서를 의식한 주석이 곳곳에 남아 있는 잘 다듬어진 코드다. 남은 문제는 대부분 이전 리뷰에서 지적된 뒤 아직 손대지 않은 것들이다. 이번 리뷰는 `README.md`·`.uplugin`·`*.Build.cs`·전체 Public 헤더를 읽고 `WxInventoryComponent.cpp`, `WxItemInstance.cpp`, `WxItemPickup.cpp`, `WxRewardLibrary.cpp`, `WxEquipmentComponent.cpp`, `WxGameFeatureAction_AddInventoryItems.cpp`, StateTree 태스크 2종을 정독했으며, 계약 확인용으로 `Source/WxGame` 의 소비자 코드를 교차 조회했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 FastArray 수신 콜백이 널 검사보다 먼저 `LastObservedCount` 를 갱신해 획득 델타가 영구 유실된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:83`, `:85`, `:104-107`
- **범주**: 버그/정확성
- **문제**: `PostReplicatedAdd` 는 83행에서 `Entry.LastObservedCount = Entry.StackCount;` 를 무조건 수행한 뒤 85행에서야 `Entry.Instance` 널 검사를 한다. 클라이언트가 엔트리를 받는 시점에 서브오브젝트 참조(`Entry.Instance`)가 아직 NetGUID 미해결이면 통지는 건너뛰는데 관찰값만 최신으로 올라간다. 포인터가 나중에 해결돼도 FastArray 는 그 항목을 다시 dirty 로 보지 않아 `PostReplicatedChange` 가 오지 않고, 온다 해도 `Delta == 0` 이라 107행에서 걸러진다. 즉 그 슬롯의 **획득 델타 1회가 복구 불가능하게 사라진다**. `PostReplicatedReceive` → `OnInventoryContentsChanged`(120-127행)와 `UWxItemInstance::HandleItemDefReplicated` 가 "현재 상태를 다시 읽어라" 신호를 보내므로 목록 **표시** 자체는 뒤늦게 따라잡지만, 델타로 획득 연출·토스트·누적 통계를 만드는 구독자는 그 아이템에 대해 아무것도 받지 못한다. `PostReplicatedChange`(104-105행)도 `Entry.Instance` 가 널일 때 같은 형태로 델타를 삼킨다.
- **제안**: `LastObservedCount` 갱신을 실제로 통지를 발행한 경로 안(널 검사 이후)으로 옮긴다. 그러면 미해결 슬롯은 다음 변경 때 누적 델타로 복구된다.
- **확신도**: 중간

### 2. 🟡 `AddEntry` 가 가상 확장점 호출 너머까지 배열 참조를 붙들고 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:136-151`
- **범주**: 버그/정확성
- **문제**: 136행 `Entries.AddDefaulted_GetRef()` 로 얻은 `FWxInventoryEntry& NewEntry` 를 들고 142-148행에서 `Fragment->OnInstanceCreated(NewEntry.Instance)` 를 호출하고, 그 뒤 150행 `MarkItemDirty(NewEntry)`·151행 `return NewEntry.Instance` 가 이어진다. `OnInstanceCreated` 는 README 가 공식 확장점으로 광고하는 가상 함수인데(`Public/Items/WxItemFragment.h:40`), 그 안에서 같은 인벤토리에 아이템이 추가되면 `Entries` 가 재할당돼 `NewEntry` 가 댕글링 참조가 되고 150행이 해제된 메모리를 건드린다. 현재 유일한 구현체(`UWxItemFragment_Charges`)는 충전량만 세팅해 발현되지 않지만, "인스턴스 초기 상태 주입" 용도로 프래그먼트가 하나만 늘어도 재현 가능한 형태다. 이전 리뷰(`491dd7ec`)에서 지적된 뒤 그대로다.
- **제안**: `const int32 NewIndex = Entries.AddDefaulted();` 로 인덱스를 잡고, 프래그먼트 호출 이후 `Entries[NewIndex]` 로 다시 접근한다. 확장점 계약은 그대로 둔 채 한 줄 수준으로 막힌다.
- **확신도**: 중간

### 3. 🟡 장비 경로는 여전히 배선만 있고, 그 복제 비용은 전 캐릭터가 낸다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:14`, `:34-57`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h:181`, `:236`, `Source/WxGame/Character/WxCharacterBase.cpp:43`
- **범주**: 설계/구조
- **문제**: `UWxEquipmentComponent::EquipItem` 의 유일한 호출부인 `UWxInventoryComponent::EquipItemByDef` 를 부르는 곳이 프로젝트 전체에 0건이라(헤더 236행이 스스로 그렇게 적고 있다) `EquippedItemDef` 는 항상 널이고 `ApplyEquipEffects`/`RemoveActiveEquipEffects` 는 도달 불가다. 그런데 `WxCharacterBase.cpp:43` 이 모든 캐릭터(플레이어·적·미니언)에 이 컴포넌트를 기본 서브오브젝트로 만들고 컴포넌트는 `SetIsReplicatedByDefault(true)`(14행)라, 액터마다 쓰이지 않는 복제 등록 슬롯을 하나씩 계속 소모한다. `UWxInventoryComponent::RemoveItemInstance`(181행)도 호출부 0건이다. (이전 리뷰가 함께 지적한 README 의 `OnEquipVisualChanged` 오기는 현재 README 에서 "미구현"으로 정정되어 해소됐다.)
- **제안**: 장비 기능을 곧 쓸 계획이 없으면 `UWxEquipmentComponent`·`EquipItemByDef`·`RemoveItemInstance`·`UWxItemFragment_Equippable` 을 걷어낸다. 유지한다면 최소한 `WxCharacterBase` 의 무조건 생성만이라도 거둔다(장비를 실제로 쓰는 캐릭터에만 부착).
- **확신도**: 높음

### 4. 🟡 `OnInventoryStackChanged` 의 발행 단위가 경로마다 달라 델타 계약이 없다 — 게다가 발행마다 전체 재스캔
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:297`, `:313`, `:406`, `:612-621`
- **범주**: 설계/구조
- **문제**: 같은 델리게이트가 세 가지 서로 다른 입자도로 발행된다 — 추가 경로는 머지·분할 슬롯 하나마다 1회(297·313행), 소비 경로는 배치 전체에 1회(406행, `-NumToConsume`), 클라이언트 복제 경로는 엔트리 하나마다 1회(83-90·101-112행). 결과적으로 "5개 지급"이 서버에선 N회, "5개 소비"가 서버에선 1회·클라에선 슬롯 수만큼 발행된다. `NewCount` 는 항상 사후 총량이라 그것만 읽는 현재 뷰모델은 맞게 동작하지만, `Delta` 를 누적하거나 발행 횟수로 연출을 트리거하는 구독자가 생기는 순간 스탠드얼론과 네트워크 플레이에서 동작이 갈린다. 여기에 `NotifyStackChangedFromList` 는 발행할 때마다 619행에서 `GetTotalItemCountByDefinition` 으로 엔트리 전체를 훑으므로 대량 지급 시 O(N·Entries) 재스캔과 구독자 콜백 증폭이 겹친다.
- **제안**: 발행 단위를 "변경 배치당 1회"로 통일한다 — `AddItemDefinition` 은 누적 델타를 모아 함수 끝에서 한 번만 발행하고(소비 경로가 이미 그 형태다), 복제 콜백도 배치 단위로 묶는다. 슬롯 단위 정보는 `OnInventorySlotChanged` 가 이미 담당한다. 통일이 어렵다면 최소한 "`NewCount` 는 항상 사후 총량, `Delta` 는 부분값일 수 있다"를 헤더 111행 주석에 못 박아 구독자 계약을 고정한다.
- **확신도**: 중간

### 5. 🟢 `MaxStack` 이 0 이하면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:272`, `:306-324`
- **범주**: 성능/안전
- **문제**: 308행 `ChunkCount = FMath::Min(MaxStack, Remaining)` 이 0 이면 319행의 `Remaining -= ChunkCount` 가 진전을 못 만들어, while 루프가 매 회전마다 새 엔트리와 `UWxItemInstance` 를 만들며 영원히 돈다(행 + OOM). `UWxItemFragment_Stackable::MaxStack` 의 `ClampMin = "1"`(`Public/Items/WxItemFragment.h:109`)은 디테일 패널 입력만 막지, 이전에 저장된 값이나 툴셋·스크립트로 직접 쓰인 값은 막지 못한다. 이전 리뷰에서 지적된 뒤 그대로다.
- **제안**: 272행을 `const int32 MaxStack = FMath::Max(1, Stackable ? Stackable->MaxStack : 1);` 로 한 번 방어한다.
- **확신도**: 중간

### 6. 🟢 `GrantReward` 가 `World` 만 검사 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:43`, `:75`
- **범주**: 성능/안전
- **문제**: `SourceActor`·`RewardRow`·`Row`·`ItemDef`·`ItemActorClass`·`SpawnedPickup` 을 모두 방어적으로 검사하는 함수에서 `UWorld* World = SourceActor->GetWorld();`(43행)만 검사 없이 75행에서 `World->SpawnActorDeferred` 로 넘어간다. BP 에 노출된 유일한 진입점이라 CDO·정리 중인 액터가 들어올 여지가 있고, 다른 방어와 일관되지도 않다.
- **제안**: 43행 뒤에 `if (!World) { return; }` 를 붙인다.
- **확신도**: 중간

### 7. 🟢 GameFeatureAction 이 `Super::OnGameFeatureActivating/Deactivating` 을 호출하지 않는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxGameFeatureAction_AddInventoryItems.cpp:13-16`, `:39-46`
- **범주**: 규칙 위반
- **문제**: 두 override 모두 부모 구현을 부르지 않는다. UE 5.8 의 `UGameFeatureAction` 기본 구현은 레거시 무인자 오버로드로 넘기는 정도라 지금은 눈에 띄는 영향이 없지만, Lyra 를 포함한 엔진 표준 액션은 모두 `Super::` 를 부른다 — 엔진이 베이스에 로직을 추가하는 순간 조용히 깨진다. 이 모듈의 다른 override(`EndPlay`, `GetLifetimeReplicatedProps`, `PostEditChangeProperty`)는 전부 `Super::` 를 부르고 있어 여기만 예외다.
- **제안**: 두 함수 첫 줄에 `Super::OnGameFeatureActivating(Context);` / `Super::OnGameFeatureDeactivating(Context);` 를 추가한다.
- **확신도**: 높음

### 8. 🟢 같은 클래스 안에서 OnRep 함수 명명이 갈린다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h:73`, `:76`, `:79`, `:82`
- **범주**: 규칙 위반
- **문제**: `UWxItemInstance` 의 두 `ReplicatedUsing` 콜백이 각각 `HandleItemDefReplicated`(73행)와 `OnRep_CurrentCharges`(76행)로 서로 다른 규약을 쓴다. CLAUDE.md 규칙 4 의 `Handle` 접두는 델리게이트 콜백 대상이라 OnRep 은 엄밀히 위반은 아니지만, 한 클래스 안에서 두 규약이 섞여 있어 `OnRep_` 으로 검색하는 다음 독자가 절반만 찾는다. 모듈의 다른 OnRep(`AWxItemPickup::OnRep_ItemDef`)은 `OnRep_` 쪽이다.
- **제안**: 모듈 다수파인 `OnRep_ItemDef` 로 통일한다(`HandleItemDefReplicated` → `OnRep_ItemDef`).
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxGameFeatureAction_AddInventoryItems.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`
- **훑은 파일**: `Plugins/WxInventory/README.md`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `.../WxStateTreeTask_RefillItemCharges.cpp`, `.../Private/Items/WxItemDefinition.cpp`, `.../Private/Items/WxItemFragment.cpp`, `.../Private/Items/WxRewardTableRow.cpp`, `.../Private/WxInventoryModule.cpp`, 나머지 Public 헤더 전부. 계약 교차 확인으로 `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`
- **미검토 / 한계**:
  - StateTree 태스크 2종이 대상 인벤토리를 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 고정하는 점(`WxStateTreeTask_GiveRewards.cpp:40`, `WxStateTreeTask_RefillItemCharges.cpp:36`)은 이전 리뷰 판단대로 모듈 고유 결함이 아니라 프로젝트 전반의 싱글플레이 전제라 발견으로 올리지 않았다. 멀티 정책이 정해지는 시점에 WxQuest·WxDialogue 와 함께 봐야 한다.
  - "Pawn → PlayerController → `FindComponentByClass<UWxInventoryComponent>`" 관용구가 5곳에 복제돼 있으나(`WxItemPickup.cpp:88`, `WxRewardLibrary.cpp:39`, `WxStateTreeTask_RefillItemCharges.cpp:36`, `WxItemInstance.cpp:85`·`:95`), 공용 정적 헬퍼는 과거에 의도적으로 제거된 것이라 재제안하지 않았다.
  - 서버 권한 API 의 `check(HasAuthority())` 계약(`WxInventoryComponent.cpp:269`, `:353`, `:387`, `:505`, `:569`, `:586`)은 Lyra 식 계약을 그대로 따른 것으로 보아 판단하지 않았다. 현재 유일한 외부 호출 경로(`UWxItemUseComponent::HandleUseItemEvent`)가 앞단에서 권한을 거르는 것은 확인했다.
  - 발견 1·4 는 실제 델타 패킷 순서에 달린 문제라 PIE 리슨/데디 조합 실측 없이 코드 경로 추론에 근거한다. 이 환경에서는 빌드·에디터 실행이 불가능해 컴파일·런타임 검증을 하지 않았다.
  - 아이템 정의·보상 DataTable 등 데이터 에셋의 실제 값(`MaxStack`, `ItemActorClass` 설정 여부 등)과 BP/WBP 내부 구조는 열어보지 않았다.

---
*문서 기준 커밋 `6ea7624` · 리뷰일 2026-09-06 · 소스 24파일 — `/module-review`로 갱신*
