# WxInventory — 코드 리뷰

> 데이터 주도 아이템 정의 + FastArray 서버 권위 인벤토리라는 골격은 여전히 견고하고, 코딩·모듈 규칙 위반은 이번에도 한 건도 없다(Copyright 첫 줄 22/22, `Wx` 접두사, 람다 0건, 인라인 정의 0건(예외 2종은 사유 주석 있음), `BlueprintCallable` 은 `UWxRewardLibrary` 팩토리 한 곳뿐, override 의 `Super::` 호출 정상, 의존성은 `WxCore` + 엔진 플러그인만). 남은 문제는 에셋 매니저 설정 불일치, 대상 플레이어 하드코딩, 데이터 오설정에 대한 무방비, 그리고 배선되지 않은 채 남은 장비 경로다. 이번 리뷰는 22개 소스를 전부 훑고 인벤토리 컴포넌트·복제 콜백·보상/픽업 경로 cpp 를 라인 단위로 읽었으며, 복제 순서와 PrimaryAssetId 판정은 UE 5.8 엔진 소스(`DataChannel.cpp`, `AssetManager.cpp`, `DataAsset.cpp`, `Class.cpp`)로 직접 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 `GetPrimaryAssetId()` 가 돌려주는 타입이 AssetManager 설정에 등록된 타입과 다르다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp:12-15`, `Config/DefaultGame.ini:50`
- **범주**: 버그/정확성
- **문제**: 코드는 `FPrimaryAssetId(TEXT("WxItem"), GetFName())` 을 반환하는데, 설정은 같은 자산을 `PrimaryAssetType="WxItemDefinition"`(`AssetBaseClass=/Script/WxInventory.WxItemDefinition`, `Directories=/Game/Item`)로 스캔 등록한다. `/Game/Item` 에 실제 `DA_Gold`·`DA_Katana`·`DA_Potion` 이 있으므로 두 아이디가 매 자산마다 어긋난다. 엔진은 이 상태를 명시적으로 에러로 잡는다 — `UAssetManager::OnObjectPreSave`(엔진 `AssetManager.cpp:5486-5495`) 가 `bIsEditorOnly=False` 타입에 대해 `"Registered PrimaryAssetId ... does not match object's real id of ...! This will not load properly at runtime!"` 를 Error 로 남긴다. 즉 디자이너가 아이템 자산을 저장할 때마다 에러가 찍히고, `ItemDef->GetPrimaryAssetId()` 로 얻은 아이디로 `LoadPrimaryAsset`/`GetPrimaryAssetPath` 를 부르는 경로는 조용히 실패한다. 게다가 `Config/DefaultGame.ini` 의 `bShouldManagerDetermineTypeAndName=False` 라 엔진은 여러 지점에서 객체가 스스로 답한 아이디를 신뢰한다.
- **제안**: 오버라이드를 삭제한다. `UPrimaryDataAsset::GetPrimaryAssetId()` 의 네이티브 자산 기본 구현이 정확히 `FPrimaryAssetId(GetClass()->GetFName(), GetFName())` = `WxItemDefinition:<자산명>` 이라 설정과 그대로 맞는다(엔진 `DataAsset.cpp:120-122`). 굳이 `WxItem` 이라는 짧은 타입명을 유지해야 한다면 `DefaultGame.ini` 의 `PrimaryAssetType` 을 `WxItem` 으로 바꿔 한쪽으로 통일한다.
- **확신도**: 높음

### 2. 🟡 StateTree 보상·리필 태스크가 대상 플레이어를 0번으로 하드코딩한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:40`, `Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:36`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 대상을 고른다. 모듈 전체가 서버 권위 + FastArray 복제로 설계돼 있는데(README 의 "리플리케이션/권한" 절), 정작 보상 직접 지급(재화 등 Pickup Fragment 없는 항목)과 에스트병 리필은 항상 서버가 아는 0번 컨트롤러에게만 간다. 두 번째 플레이어가 상자를 열거나 화톳불을 쓰면 보상·리필이 남에게 흘러간다.
- **제안**: 기믹을 발동한 액터(상호작용 Interactor)를 StateTree 파라미터로 바인딩해 대상으로 넘긴다. `AWxDevice` 계열은 `OnInteracted` 에서 이미 Interactor 를 쥐고 있으므로 발행만 하면 된다.
- **확신도**: 중간(1인 플레이 전제의 의도된 단순화일 수 있음 — `WxStateTreeTask_RefillItemCharges.h:131` 주석이 "로컬 플레이어(0번 컨트롤러)"라고 명시한다)

### 3. 🟡 비-Stackable 아이템 대량 지급이 상한 없이 N개 인스턴스를 만들고 청크마다 전량 재스캔한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:305-323`, `:296`, `:618`
- **범주**: 성능/안전
- **문제**: Stackable Fragment 가 없으면 `MaxStack = 1` 이라 `AddItemDefinition(Def, N)` 의 `while` 루프가 `NewObject` + `AddReplicatedSubObject` 를 N회 수행한다(:305-323). 게다가 청크마다 호출되는 `NotifyStackChangedFromList` 가 매번 `GetTotalItemCountByDefinition`(:618)으로 전체 엔트리를 훑어 총 O(N²)이 되고, 머지 루프(:296)도 같은 패턴이다. `GrantReward`/`GrantItems` 는 DataTable 의 `Quantity` 를 검증 없이 그대로 넘기므로, 디자이너가 Stackable 없는 아이템(예: 장비)에 큰 수량을 넣으면 지급 한 번에 프레임이 멈추고 복제 서브오브젝트가 폭증한다. 코드 어디에도 상한이 없다.
- **제안**: `AddItemDefinition` 진입부에 `StackCount` 상한(또는 Stackable 부재 시 수량 클램프 + 경고 로그)을 두고, 통지는 루프 밖에서 합산 델타 1회로 모은다.
- **확신도**: 높음(코드 형태는 확정. 실제 발현은 데이터 오설정에 달려 있다)

### 4. 🟡 픽업 비주얼을 게임스레드에서 동기 로드하고, 데디케이티드 서버에서도 로드한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:52-57`, `:116-137`
- **범주**: 성능/안전
- **문제**: `SetItemDef` 가 곧바로 `ApplyPickupVisual()` 을 부르고(:56), 거기서 StaticMesh 와 NiagaraSystem 을 `LoadSynchronous`(:129, :134) 한다. 드랍은 전투 중 다발적으로 발생하므로 새 에셋을 만나는 첫 드랍마다 게임스레드 히치가 난다. 더구나 `SetItemDef` 는 `UWxRewardLibrary::GrantReward` 의 서버 스폰 경로에서만 불리는데, 데디케이티드 서버에는 메시·나이아가라가 전혀 필요 없다(클라는 `OnRep_ItemDef`(:111)로 따로 로드한다). 헤더 주석은 "서버 권한에서만 호출"이라 하지만 코드에는 권한 가드도 없다.
- **제안**: `ApplyPickupVisual` 초입에 `IsRunningDedicatedServer()` 게이트를 둔다. 드랍이 잦은 아이템은 보상 테이블 로드 시점에 비동기 프리로드해 히치를 없앤다.
- **확신도**: 중간

### 5. 🟡 장비 경로 전체가 호출부 0건인 채로 남아 있고, 배선하는 순간 클라 초기 외형이 유실된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:583`(`EquipItemByDef`), `:345`(`RemoveItemInstance`), `Private/Inventory/WxEquipmentComponent.cpp:34-62`
- **범주**: 중복/복잡도
- **문제**: `EquipItemByDef` 는 프로젝트 전역에서 호출부가 없고 `BlueprintCallable` 도 아니라 BP 진입도 불가하다. 그 유일한 소비자 `UWxEquipmentComponent::EquipItem` 도 마찬가지다. 그런데 `AWxCharacterBase` 는 전 캐릭터에 `EquipmentComponent` 를 CDO 로 붙이고(`Source/WxGame/Character/WxCharacterBase.cpp:45`) `OnEquipVisualChanged` 를 BeginPlay 에서 구독까지 한다(:91) — 항상 null 인 `EquippedItemDef` 를 복제하는 컴포넌트가 모든 캐릭터에 달려 있는 셈이다. `RemoveItemInstance` 도 동일하게 호출부 0건이다. 그리고 배선을 마치는 순간 헤더가 스스로 지적한 문제(`WxEquipmentComponent.h:28`)가 터진다 — 클라에서 네트워크 스폰 액터의 초기 RepNotify 는 `BeginPlay` 보다 먼저 도는데 구독은 `BeginPlay` 에서 하므로, 이미 장비를 낀 캐릭터가 relevant 해진 클라는 `OnRep_EquippedItemDef` 방송을 놓치고 현재 상태를 되물을 pull API 가 없어 영구히 맨손으로 보인다.
- **제안**: 장비 경로를 실제로 배선하되, 함께 `GetEquippedItemDef()`(혹은 구독 시 즉시 현재 값을 밀어주는 헬퍼)를 추가해 late-join 유실을 닫는다. 당분간 쓰지 않을 것이면 `RemoveItemInstance` 와 함께 컴포넌트 부착까지 걷어내 복제 비용과 오해를 없앤다.
- **확신도**: 높음(데드 코드 사실). late-join 유실은 중간(경로가 죽어 있어 현재는 잠재적)

### 6. 🟢 `UseItemByDef` 가 인스턴스를 인벤토리에서 떼어낸 뒤 그 인스턴스를 SourceObject 로 가진 GE 를 적용한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:529-537`, `:548`, `:553-556`
- **범주**: 버그/정확성
- **문제**: `Context.AddSourceObject(SourceInstance)`(:531)로 Spec 을 먼저 만들고, 비-충전형 경로에서 `ConsumeItemsByDefinition(ItemDef, 1)`(:548)이 마지막 스택을 소진하면 엔트리가 제거되어 `SourceInstance` 의 유일한 UPROPERTY 참조가 사라진다. 그 뒤 `ApplyGameplayEffectSpecToSelf`(:555). `FGameplayEffectContext::SourceObject` 는 약참조라, Duration/Infinite GE 라면 다음 GC 이후 SourceObject 가 null 이 되어 `UWxItemInstance.h:19` 가 명시한 목적("효과 측이 인스턴스별 데이터에 접근하는 진입점")이 무너진다. Instant GE 는 즉시 실행이라 무해하고, 현재 C++ 쪽에 `GetSourceObject()` 소비자는 없다.
- **제안**: SourceObject 로 `UWxItemInstance` 대신 `UWxItemDefinition` 을 싣거나, 소비 대상 인스턴스를 GE 적용이 끝날 때까지 강참조로 붙들어 둔다.
- **확신도**: 중간(현재 소비 아이템이 Instant GE 뿐이면 표면화되지 않음)

### 7. 🟢 `Usable` Fragment 에 `Effect` 가 비어 있으면 아이템이 아무 일 없이 소모된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:518-538`, `:540-551`
- **범주**: 버그/정확성
- **문제**: `if (Usable->Effect)`(:520) 가 false 면 `TargetASC`/`Spec` 이 빈 채로 통과하고, 그대로 충전 차감 또는 스택 차감이 실행된다(:540-551). GE 를 지정하지 않은 Usable 아이템은 아무 효과 없이 사라지며 `UseItemByDef` 는 `true` 를 반환한다 — 데이터 오설정이 런타임에 조용한 아이템 손실로만 드러난다.
- **제안**: `Effect` 미지정이면 조기에 `false` 를 반환하거나, 최소한 경고 로그를 남긴다.
- **확신도**: 높음

### 8. 🟢 `GrantReward` 가 `World` 널 검사와 스폰 후 유효성 검사를 하지 않는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:38`, `:70`, `:77-79`
- **범주**: 버그/정확성
- **문제**: `SourceActor->GetWorld()`(:38) 결과를 검사 없이 `World->SpawnActorDeferred`(:70)에 쓴다. 또 `FinishSpawning`(:77)이 픽업 BP 의 BeginPlay 를 돌리는데 거기서 액터가 파괴될 수 있고, 그 뒤 `SpawnedPickup->LaunchInDirection`(:79)을 무조건 호출한다.
- **제안**: `World` 널 가드를 추가하고, `LaunchInDirection` 앞에 `IsValid(SpawnedPickup)` 검사를 넣는다.
- **확신도**: 중간(권위 액터라 `World` 가 null 일 확률 자체는 낮다)

### 9. 🟢 `UWxItemFragment_Charges` 의 에디터 표시 이름만 "Refill" 이다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h:85`
- **범주**: 중복/복잡도
- **문제**: 클래스명·README·주석·`RefillItemCharges` 외 모든 코드가 이 축을 "Charges" 로 부르는데 `UCLASS(DisplayName = "Refill")` 만 다르다. 나머지 5종 Fragment 는 모두 DisplayName 이 클래스명과 일치해(`Equippable`/`Usable`/`Stackable`/`Pickup`/`Grade`) 이것만 튄다 — 디자이너가 Fragment 목록에서 문서와 매칭하지 못한다.
- **제안**: `DisplayName = "Charges"` 로 통일한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Public/Inventory/WxInventoryComponent.h`, `Private/Items/WxItemPickup.cpp`, `Private/WxRewardLibrary.cpp`, `Private/Inventory/WxEquipmentComponent.cpp`, `Private/Items/WxItemInstance.cpp`, `Private/Items/WxItemDefinition.cpp`
- **훑은 파일**: `Public/Items/WxItemFragment.h`, `Private/Items/WxItemFragment.cpp`, `Public/Items/WxItemDefinition.h`, `Public/Items/WxItemInstance.h`, `Public/Items/WxItemPickup.h`, `Public/Items/WxRewardTableRow.h`, `Private/Items/WxRewardTableRow.cpp`, `Public/WxRewardLibrary.h`, `Public/Inventory/WxEquipmentComponent.h`, `Public/Inventory/WxStateTreeTask_GiveRewards.h`, `Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Public/Inventory/WxStateTreeTask_RefillItemCharges.h`, `Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `Public/WxInventoryModule.h`, `Private/WxInventoryModule.cpp`, `WxInventory.Build.cs`, `WxInventory.uplugin`
- **교차 확인**: 호출부·소비자 검증을 위해 `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Config/DefaultGame.ini` 를 함께 읽었다
- **이전 리뷰(`e9630dc2`)에서 정정한 항목**:
  - "복제 콜백이 미해석 `Instance` 를 통지 없이 삼킨다"(구 발견 1)는 **성립하지 않는다**. UE 5.8 `UActorChannel::ReplicateRegisteredSubObjects`(`DataChannel.cpp:4216-4229`)는 컴포넌트의 등록 서브오브젝트를 먼저 쓰고 컴포넌트 자신을 맨 마지막에 쓴다(엔진 주석: "SubObjects have to be created before the component on the receiving end"). 본 컴포넌트는 `bReplicateUsingRegisteredSubObjectList = true` 이므로 `PostReplicatedAdd` 시점에 `UWxItemInstance` 는 이미 클라에 존재한다. `if (Entry.Instance)` 가드는 방어적 코드일 뿐이다.
  - "권위 가드가 `check()` 라 Shipping 에서 사라진다"(구 발견 2)는 **실질 위험이 없어 제외**했다. 인벤토리 변경 진입점에 도달하는 외부 경로(`UWxRewardLibrary::GrantReward`, `AWxItemPickup::OnInteracted` ← ServerOnly 인 `UWxAbility_Interact`, `UWxItemUseComponent::HandleUseItemEvent`, StateTree 태스크 2종)가 모두 자체 `HasAuthority` 게이트를 갖고 있어 클라가 닿을 수 없다. 프로그래머 오류용 assert 로 타당하다.
  - "머지 루프 중 브로드캐스트 재진입"(구 발견 6)도 **제외**했다. 현 구독자(`UWxViewModel_Inventory`/`UWxViewModel_Item`)가 통지 처리 중 인벤토리를 변경하지 않으며, `Entries` 는 데이터 포인터가 아니라 배열 객체 참조라 재할당에도 안전하다.
- **미검토 / 한계**:
  - 발견 1의 실제 에러 로그 발현은 에디터에서 아이템 자산을 저장해 확인하지 않았다(엔진 소스 조건 대조까지만 했다).
  - `.uasset` 실데이터(아이템 정의의 Fragment 조합, 보상 DataTable 의 Quantity, 픽업 BP)는 확인하지 않았다 — 발견 3·7 은 데이터 오설정 시의 위험이라 실 데이터 점검이 함께 필요하다.
  - 인벤토리 델리게이트 구독자(`Source/WxGame/MVVM/WxViewModel_*`)의 수명·해제 정확성은 WxGame 리뷰 범위로 넘겼다.

---
*문서 기준 커밋 `c486a5c7` · 리뷰일 2026-09-03 · 소스 22파일 — `/module-review`로 갱신*
