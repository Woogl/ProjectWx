# WxInventory — 코드 리뷰

> 데이터 모델(Definition + Fragment 컴포지션)과 서버 권위 복제 경계가 잘 잡혀 있고, 코딩·모듈 규칙 위반은 기계적으로 확인한 범위에서 0건인 건강한 모듈이다. 이번 리뷰는 `FWxInventoryList` 복제 콜백과 `UWxInventoryManagerComponent` 의 Add/Consume/Use 경로, `UWxEquipmentComponent` 의 GE 수명 관리를 라인 단위로, 나머지(Fragment/Instance/Pickup/Reward/StateTree 태스크)는 22개 파일을 전부 읽되 로직 밀도가 낮은 곳은 훑는 수준으로 봤다. 직전 리뷰(`13b45192`) 이후 변경분은 주석 2줄뿐이라(`WxItemPickup.cpp` 의 `IsInteractionEnabled`→`CanInteract` 정정, `WxItemInstance.h` 의 규칙 6 예외 사유 추가) 코드 상 발견은 이전과 동일하게 유지된다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 클라이언트에서 아이템 추가 통지가 통째로 유실될 수 있다 (unmapped GUID 복구 경로를 스스로 막음)
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:83`, 같은 파일 `:20`, `:104`
- **범주**: 버그/정확성
- **문제**: `PostReplicatedAdd` 가 `Entry.LastObservedCount = Entry.StackCount;`(83행)를 `if (Entry.Instance)` 가드(85행) **바깥**에서 먼저 수행한다.
  `UWxItemInstance` 는 매니저 컴포넌트의 등록 서브오브젝트라, 액터 채널은 컴포넌트의 프로퍼티 블록(= `InventoryList`)을 **먼저** 쓰고 그 컴포넌트의 서브오브젝트를 **나중에** 쓴다. 따라서 신규 엔트리가 처음 도착하는 순간 `Entry.Instance` 의 NetGUID 는 아직 unmapped 이고 `Entry.Instance == nullptr` 이 되어 85~89행의 슬롯/합계 통지가 통째로 스킵된다(엔트리의 `ItemDef` 가 클라에 아직 로드되지 않은 하드 참조라면 `GetItemDef()` 도 같은 이유로 null 이 되어 `NotifyStackChangedFromList` 만 조용히 조기 반환한다).
  엔진은 이 상황을 위해 GUID 가 해석되면 `FastArrayDeltaSerialize_DeltaSerializeStructs` 의 `bUpdateUnmappedObjects` 분기에서 `ArraySerializer.PostReplicatedChange(ChangedIndices, ...)` 를 다시 호출해 준다(UE 5.8 `FastArraySerializer.h`). 그런데 그 시점엔 이미 `LastObservedCount == StackCount` 이므로 104행의 `Delta` 가 0 이 되고, 107행 `Delta != 0` 가드에 걸려 복구 통지마저 삼켜진다. 결과적으로 리슨/데디 서버의 원격 클라에서는 획득한 아이템이 `OnInventorySlotChanged`/`OnInventoryStackChanged` 를 한 번도 발행하지 않아 인벤토리 UI·뷰모델(`Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `WxViewModel_Item.cpp`)이 그 아이템을 영영 모른다(스탠드얼론·리슨서버 로컬에서는 서버 경로가 직접 발행하므로 재현되지 않는다 — 원격 클라 전용 증상이다).
- **제안**: `FWxInventoryEntry` 의 `LastObservedCount` 기본값을 `INDEX_NONE`(20행) 대신 `0` 으로 바꾸고, `PostReplicatedAdd` 에서 `LastObservedCount` 갱신을 실제로 통지를 발행한 `if (Entry.Instance)` 블록 안으로 옮긴다. 그러면 늦게 해석된 엔트리는 뒤이은 `PostReplicatedChange` 에서 `Delta = StackCount - 0` 으로 정상 발행된다. 기본값을 0 으로 바꾸는 것은 `PreReplicatedRemove`(60행)의 `-Entry.LastObservedCount` 가 미관측 엔트리에서 `+1` 이라는 잘못된 부호를 내는 것도 함께 막는다.
- **확신도**: 중간 (콜백 순서·엔진 복구 경로는 소스로 확인했으나, 실제 패킷에서 `Entry.Instance` 가 unmapped 로 도착하는지는 네트워크 PIE 실측이 필요하다)

### 2. 🟡 StateTree 보상/리필 태스크가 0번 플레이어 컨트롤러로 고정돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:41`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:37`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 `UGameplayStatics::GetPlayerController(Owner, 0)` 을 지급/리필 대상으로 쓴다. 모듈 전체가 FastArray 델타 복제·서버 권위·`HasAuthority()` 게이팅으로 멀티플레이를 전제하고 만들어졌는데, 정작 대상 선택만 싱글플레이 가정이다. 데디케이티드 서버에서 `GetPlayerController(World, 0)` 은 "로컬 플레이어"가 아니라 컨트롤러 이터레이터의 첫 항목이라, 체크포인트 리필이 임의의 한 명에게만 걸리고 보상도 그 한 명에게만 직접 지급된다. `FWxStateTreeTask_RefillItemCharges` 헤더 주석(`WxStateTreeTask_RefillItemCharges.h:21`)이 "로컬 플레이어(0번 컨트롤러)"라고 적고 있어 의도된 단순화로 보이지만, 서버에는 로컬 플레이어라는 개념이 없다.
- **제안**: 대상 선택을 태스크 인스턴스 데이터의 바인딩 가능한 Actor 파라미터로 노출하거나(호출 측 ST 가 대상을 지목), 최소한 리필은 `GetWorld()->GetPlayerControllerIterator()` 로 전 플레이어를 순회한다. 지금 형태를 유지한다면 "싱글플레이 전용 태스크"임을 헤더에 명시해 두는 것이 낫다.
- **확신도**: 중간 (프로젝트가 의도적으로 싱글플레이 우선일 수 있다)

### 3. 🟡 장비 경로 전체가 호출부 0건인 데드 코드인데 소비자만 붙어 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h:236`(`EquipItemByDef`), 같은 헤더 `:180`(`RemoveItemInstance`), `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:34`(`EquipItem`)
- **범주**: 중복/복잡도
- **문제**: `EquipItemByDef` 는 저장소 전체에서 호출부가 0건이고, 그 유일한 하위 호출인 `UWxEquipmentComponent::EquipItem` 역시 `EquipItemByDef` 외 진입이 없다. 둘 다 `BlueprintCallable` 이 아니라 BP 진입도 불가하다. `RemoveItemInstance` 도 동일하게 0건이다. 그런데 소비자 쪽은 이미 배선돼 있어(`Source/WxGame/Character/WxCharacterBase.cpp:83` 이 `OnEquipVisualChanged` 를 구독) "붙어 있는데 아무 일도 안 일어나는" 상태다. 이 상태가 지속되면 `EquippedItemDef` 복제·`ActiveEquipEffectHandles` 수명 관리·`BroadcastEquipVisual` 이 전부 미검증인 채 남는다. 겸해서 `ApplyEquipEffects`(같은 파일 `:89`)는 ASC 해석에 실패하면 조용히 반환하는데 `EquippedItemDef` 는 이미 세팅된 뒤라(51행) 이후 ASC 가 준비돼도 재적용 기회가 없다 — 경로를 살릴 때 함께 닫아야 할 구멍이다.
- **제안**: 트리거를 실제로 배선하거나(인벤토리 UI/입력 → `EquipItemByDef`), 배선 계획이 없다면 `UWxEquipmentComponent` 구독까지 포함해 통째로 걷어내 죽은 표면을 줄인다. 살리는 쪽을 택하면 ASC 미준비 시 `EquippedItemDef` 를 확정하지 않거나, 폰의 ASC 초기화 시점에 재적용하는 훅을 추가한다.
- **확신도**: 높음 (호출부 0건은 저장소 전역 grep 으로 확인)

### 4. 🟢 `AddItemDefinition` 이 내부 배열 참조를 든 채로 루프 안에서 델리게이트를 방송한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:278`
- **범주**: 버그/정확성
- **문제**: 278행에서 `InventoryList.GetEntries()` 의 참조를 캐시한 뒤 279~302행 루프 안에서 `NotifySlotChangedFromList`/`NotifyStackChangedFromList` 로 임의의 구독자 코드를 실행한다. 구독자가 재진입으로 `AddItemDefinition`/`ConsumeItemsByDefinition` 을 호출하면 `Entries` 가 재할당·축소되어 캐시된 참조와 `EntryIndex` 가 모두 무효가 된다(`ConsumeByDefinition` 의 `RemoveCurrent` 는 기본 `AllowShrinking` 으로 재할당까지 유발할 수 있다).
- **제안**: 머지 루프에서는 변경 결과만 모아 두고 루프 종료 후 방송하거나, 매 반복마다 `InventoryList.GetEntries()` 를 다시 얻고 `Num()` 을 재확인한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 현재 구독자는 표시용 뷰모델뿐이라 재진입 실적이 없다)

### 5. 🟢 `GetPrimaryAssetId` 오버라이드가 등록되지 않은 타입을 반환한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp:12`
- **범주**: 설계/구조
- **문제**: `FPrimaryAssetId(TEXT("WxItem"), GetFName())` 을 반환하지만 `Config/DefaultGame.ini` 의 `PrimaryAssetTypesToScan` 에 `WxItem` 타입이 없다(등록된 것은 Map/PrimaryAssetLabel/GameFeatureData/WxExperienceDefinition/WxExperienceActionSet뿐). AssetManager 가 스캔하지 않는 타입이므로 `GetPrimaryAssetIdForPath`·번들 로딩·쿡 룰 어디에도 걸리지 않고, 저장소 안에 이 ID 를 소비하는 코드도 0건이다. 지금은 무해하지만 "이 자산은 PrimaryAsset 으로 관리된다"는 잘못된 신호를 준다.
- **제안**: AssetManager 에 `WxItem` 타입을 등록해 실제로 쓰거나, 쓰지 않을 것이면 오버라이드를 제거해 베이스 동작을 그대로 둔다.
- **확신도**: 중간

### 6. 🟢 `UWxItemFragment_Charges` 의 에디터 표시 이름이 `Refill` 이라 코드·문서와 어긋난다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h:87`
- **범주**: 중복/복잡도
- **문제**: 클래스명·헤더 주석·README·매니저 API(`RefillItemCharges` 의 대상 판정)가 모두 "Charges Fragment" 로 부르는데 디테일 패널에는 `Refill` 로 뜬다. 다른 5종 Fragment 는 모두 클래스명 접미사와 `DisplayName` 이 일치한다(`Equippable`/`Usable`/`Stackable`/`Pickup`/`Grade`). 기획자가 Fragment 를 고를 때만 이름이 달라져 문서와 대조가 안 된다.
- **제안**: `DisplayName = "Charges"` 로 맞춘다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`
- **경계 확인용으로 함께 읽은 모듈 밖 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`(픽업이 구현한 계약 — `WxItemPickup.cpp:29` 주석의 `CanInteract` 정정이 실제 인터페이스와 일치함을 확인), `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:50,86`(`OnInteracted` 가 서버 권위에서만 호출되어 `AddItemDefinition` 의 `check(HasAuthority())` 가 안전함을 확인), `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp:69,77`(`UseItemByDef` 도 `HasAuthority` 가드 뒤에서만 호출됨을 확인)
- **규칙 점검 결과(위반 0건)**: 22개 소스 전부 첫 줄 저작권 표기 정상, 람다 0건, `FORCEINLINE`/자유 인라인 정의 0건, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward`(BP Function Library) 한 곳뿐, 델리게이트 바인딩(`AddUObject`/`AddDynamic` 등) 자체가 0건이라 `Handle` 접두사 대상 없음(`OnRep_*` 는 RepNotify 라 해당 없음), `Build.cs`·`uplugin` 모두 `WxCore` 외 Wx 플러그인 참조 없음, override 함수의 `Super::` 호출 누락은 의도적 대체(`IsSupportedForNetworking`/`GetPrimaryAssetId`/StateTree `EnterState`) 외 없음. 헤더 인라인 정의는 템플릿 `FindFragmentByClass<T>`(Definition/Instance)와 StateTree `GetInstanceDataType()` 뿐이며 넷 다 인접 주석에 규칙 6 예외 사유가 명시돼 있다.
- **미검토 / 한계**: (a) 발견 1의 네트워크 재현은 실측하지 않았다 — 2인 PIE(데디케이티드) 에서 아이템 획득 시 클라 UI 갱신 여부로 확인이 필요하다. (b) `AWxItemPickup` 의 물리 발사·이동 복제 동작(`SetSimulatePhysics` + `SetReplicateMovement`)은 코드 상 엔진 기본 경로에 맡기고 있어 별도 검증하지 않았다. (c) `UWxItemFragment_Pickup::ItemActorClass` 가 가리키는 픽업 BP 및 인벤토리 위젯 등 BP/WBP 내부 구조는 범위 밖이다. (d) `UWxItemFragment_Grade` 의 에디터 전용 재시드 경로는 코드만 읽고 에디터 실동작은 확인하지 않았다.

---
*문서 기준 커밋 `d359391` · 리뷰일 2026-08-25 · 소스 22파일 — `/module-review`로 갱신*
