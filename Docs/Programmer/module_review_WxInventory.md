# WxInventory — 코드 리뷰

> 데이터 주도 아이템 정의(Definition + Fragment 컴포지션)와 FastArray 서버 권위 인벤토리라는 골격은 견고하며, 코딩·모듈 규칙 위반은 이번에도 0건이다(Copyright 첫 줄 22/22, `Wx` 접두사, 람다 0건, 인라인 정의 0건 — 예외 2종은 사유 주석 있음, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward` 한 곳뿐, 의존성은 `WxCore` + 엔진 플러그인만). 지난 리뷰의 최대 항목이던 `GetPrimaryAssetId()` 불일치는 오버라이드가 제거되어 해소됐고(`DefaultGame.ini:50` 의 `WxItemDefinition` 타입과 이제 일치), 남은 문제는 대상 플레이어 하드코딩·데이터 오설정 무방비·배선되지 않은 장비 경로로 좁혀졌다. 이번 리뷰는 22개 소스를 전부 훑고 인벤토리 컴포넌트·복제 콜백·픽업/보상 경로 cpp 를 라인 단위로 읽었으며, 호출부 검증을 위해 `Source/WxGame` 과 `Plugins/WxWorld` 의 소비자·유사 태스크까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 StateTree 보상·리필 태스크가 대상 플레이어를 0번으로 하드코딩한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:40`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:36`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 `UGameplayStatics::GetPlayerController(Owner, 0)` 로 대상을 고른다. 모듈 전체가 서버 권위 + FastArray 복제로 설계돼 있는데, 정작 보상 직접 지급(Pickup Fragment 없는 재화 등)과 에스트병 리필은 서버가 아는 0번 컨트롤러에게만 간다. 두 번째 플레이어가 상자를 열거나 화톳불을 쓰면 보상·리필이 남에게 흘러간다. 같은 흐름의 WxWorld 태스크들(`FWxStateTreeTask_ApplyGameplayEffectToInteractor`, `FWxStateTreeTask_PlayInteractorMontage`)은 `AWxDevice::GetInteractingCharacter()` 로 실제 발동 주체를 쓰고 있어, 이 두 태스크만 전제가 다르다.
- **제안**: WxInventory 는 WxWorld 를 참조할 수 없으므로 `AWxDevice` 캐스팅 대신 인스턴스 데이터에 대상 `AActor*` 파라미터를 추가하고 ST 에셋에서 발동 주체를 바인딩한다. 현재 `AWxDevice::InteractingCharacter`(`Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h:77-78`)는 `UPROPERTY(Replicated, Transient)` 라 바인딩 가능한 지정자가 없어 노출 작업이 함께 필요하다.
- **확신도**: 중간(1인 플레이 전제의 의도된 단순화일 수 있음 — `WxStateTreeTask_RefillItemCharges.h:21` 주석이 "로컬 플레이어(0번 컨트롤러)"라고 명시한다. 다만 "스플릿스크린 미사용" 전제는 멀티 클라이언트를 커버하지 않는다)

### 2. 🟡 비-Stackable 아이템 대량 지급이 상한 없이 N개 인스턴스를 만들고 청크마다 전량 재스캔한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:305-323`, `:296`, `:618`
- **범주**: 성능/안전
- **문제**: Stackable Fragment 가 없으면 `MaxStack = 1`(`:271`)이라 `AddItemDefinition(Def, N)` 의 `while` 루프가 `NewObject` + `AddReplicatedSubObject` 를 N회 수행한다(`:305-323`). 게다가 청크마다 호출되는 `NotifyStackChangedFromList` 가 매번 `GetTotalItemCountByDefinition`(`:618`)으로 전체 엔트리를 훑어 총 O(N²)이 되고, 머지 루프(`:296`)도 같은 패턴이다. `UWxRewardLibrary::GrantReward`(`Private/WxRewardLibrary.cpp:53`)와 `GrantItems`(`:340`)는 DataTable 의 `Quantity` 를 검증 없이 그대로 넘기므로, 디자이너가 Stackable 없는 아이템(예: 장비)에 큰 수량을 넣으면 지급 한 번에 프레임이 멈추고 복제 서브오브젝트가 폭증한다. 코드 어디에도 상한이 없다.
- **제안**: `AddItemDefinition` 진입부에 `StackCount` 상한(또는 Stackable 부재 시 수량 클램프 + 경고 로그)을 두고, 통지는 루프 밖에서 합산 델타 1회로 모은다.
- **확신도**: 높음(코드 형태는 확정. 실제 발현은 데이터 오설정에 달려 있다)

### 3. 🟡 픽업 비주얼을 게임스레드에서 동기 로드한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:52-57`, `:116-143`
- **범주**: 성능/안전
- **문제**: `SetItemDef` 가 곧바로 `ApplyPickupVisual()` 을 부르고(`:56`), 거기서 StaticMesh 와 NiagaraSystem 을 `LoadSynchronous` 한다(`:129`, `:134`). 드랍은 전투 중 다발적으로 발생하므로 새 에셋을 만나는 첫 드랍마다 게임스레드 히치가 난다. 데디케이티드 서버에서도 같은 로드가 돈다(`SetItemDef` 는 서버 스폰 경로 전용이고, 클라는 `OnRep_ItemDef`(`:111`)로 따로 로드한다).
- **제안**: 드랍이 잦은 아이템은 보상 테이블 로드 시점에 Pickup Fragment 의 Mesh/Niagara 를 비동기 프리로드해 히치를 없앤다. **주의**: `ApplyPickupVisual` 전체를 `IsRunningDedicatedServer()` 로 막으면 안 된다 — StaticMesh 는 루트 컴포넌트의 물리 바디이자 쿼리 콜리전 형상이라, 서버에서 비면 `LaunchInDirection`(`:66-68`)의 물리 발사가 실패하고 `IWxInteractable` 계약상 스캔·사거리 판정에도 걸리지 않게 된다(`Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:16`). 서버에서 생략 가능한 건 Niagara 쪽뿐이다.
- **확신도**: 높음(동기 로드 사실). 서버 생략 범위는 중간

### 4. 🟡 장비 경로 전체가 호출부 0건인 채로 남아 있고, 배선하는 순간 클라 초기 외형이 유실된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:583`(`EquipItemByDef`), `:345`(`RemoveItemInstance`), `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:34-58`
- **범주**: 중복/복잡도
- **문제**: `EquipItemByDef` 는 프로젝트 전역에서 호출부가 없고 `BlueprintCallable` 도 아니라 BP 진입도 불가하다(전역 grep 결과 선언·정의뿐). 그 유일한 소비자 `UWxEquipmentComponent::EquipItem` 도 마찬가지고, `RemoveItemInstance` 역시 호출부 0건이다. 그런데 `AWxCharacterBase` 는 전 캐릭터에 `EquipmentComponent` 를 CDO 로 붙이고(`Source/WxGame/Character/WxCharacterBase.cpp:45`) `OnEquipVisualChanged` 를 BeginPlay 에서 구독까지 한다(`:89`) — 항상 null 인 `EquippedItemDef` 를 복제하는 컴포넌트가 모든 캐릭터에 달려 있는 셈이다. 그리고 배선을 마치는 순간 헤더가 스스로 지적한 문제(`Public/Inventory/WxEquipmentComponent.h:28`)가 터진다 — 클라에서 네트워크 스폰 액터의 초기 RepNotify 는 `BeginPlay` 보다 먼저 도는데 구독은 `BeginPlay` 에서 하므로, 이미 장비를 낀 캐릭터가 relevant 해진 클라는 `OnRep_EquippedItemDef` 방송을 놓치고 현재 상태를 되물을 pull API 가 없어 영구히 맨손으로 보인다.
- **제안**: 장비 경로를 실제로 배선하되 함께 `GetEquippedItemDef()`(혹은 구독 시 즉시 현재 값을 밀어주는 헬퍼)를 추가해 late-join 유실을 닫는다. 당분간 쓰지 않을 것이면 `RemoveItemInstance` 와 함께 컴포넌트 부착까지 걷어내 복제 비용과 오해를 없앤다.
- **확신도**: 높음(데드 코드 사실). late-join 유실은 중간(경로가 죽어 있어 현재는 잠재적)

### 5. 🟢 `UseItemByDef` 가 인스턴스를 인벤토리에서 떼어낸 뒤 그 인스턴스를 SourceObject 로 가진 GE 를 적용한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:529-537`, `:548`, `:553-556`
- **범주**: 버그/정확성
- **문제**: `Context.AddSourceObject(SourceInstance)`(`:531`)로 Spec 을 먼저 만들고, 비-충전형 경로에서 `ConsumeItemsByDefinition(ItemDef, 1)`(`:548`)이 마지막 스택을 소진하면 엔트리가 제거되어 `SourceInstance` 의 유일한 UPROPERTY 참조가 사라진다. 그 뒤 `ApplyGameplayEffectSpecToSelf`(`:555`). `FGameplayEffectContext::SourceObject` 는 약참조라, Duration/Infinite GE 라면 다음 GC 이후 SourceObject 가 null 이 되어 `Public/Items/WxItemInstance.h:19` 가 명시한 목적("효과 측이 인스턴스별 데이터에 접근하는 진입점")이 무너진다. Instant GE 는 즉시 실행이라 무해하고, 현재 C++ 쪽에 `GetSourceObject()` 소비자는 없다.
- **제안**: SourceObject 로 `UWxItemInstance` 대신 `UWxItemDefinition` 을 싣거나, 소비 대상 인스턴스를 GE 적용이 끝날 때까지 강참조로 붙들어 둔다.
- **확신도**: 중간(현재 소비 아이템이 Instant GE 뿐이면 표면화되지 않음)

### 6. 🟢 `Usable` Fragment 에 `Effect` 가 비어 있으면 아이템이 아무 일 없이 소모된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:518-538`, `:540-551`
- **범주**: 버그/정확성
- **문제**: `if (Usable->Effect)`(`:520`)가 false 면 `TargetASC`/`Spec` 이 빈 채로 통과하고, 그대로 충전 차감 또는 스택 차감이 실행된다(`:540-551`). GE 를 지정하지 않은 Usable 아이템은 아무 효과 없이 사라지며 `UseItemByDef` 는 `true` 를 반환한다 — 데이터 오설정이 런타임에 조용한 아이템 손실로만 드러난다.
- **제안**: `Effect` 미지정이면 조기에 `false` 를 반환하거나, 최소한 경고 로그를 남긴다.
- **확신도**: 높음

### 7. 🟢 `GrantReward` 가 `World` 널 검사와 스폰 후 유효성 검사를 하지 않는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:38`, `:70`, `:77-79`
- **범주**: 버그/정확성
- **문제**: `SourceActor->GetWorld()`(`:38`) 결과를 검사 없이 `World->SpawnActorDeferred`(`:70`)에 쓴다. 또 `FinishSpawning`(`:77`)이 픽업 BP 의 BeginPlay 를 돌리는데 거기서 액터가 파괴될 수 있고, 그 뒤 `SpawnedPickup->LaunchInDirection`(`:79`)을 무조건 호출한다.
- **제안**: `World` 널 가드를 추가하고, `LaunchInDirection` 앞에 `IsValid(SpawnedPickup)` 검사를 넣는다.
- **확신도**: 중간(권위 액터라 `World` 가 null 일 확률 자체는 낮다)

### 8. 🟢 `UWxItemFragment_Charges` 의 에디터 표시 이름만 "Refill" 이다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h:85`
- **범주**: 중복/복잡도
- **문제**: 클래스명·README·주석·`RefillItemCharges` 외 모든 코드가 이 축을 "Charges" 로 부르는데 `UCLASS(DisplayName = "Refill")` 만 다르다. 나머지 5종 Fragment 는 DisplayName 이 클래스명과 일치해(`Equippable`/`Usable`/`Stackable`/`Pickup`/`Grade`) 이것만 튄다 — 디자이너가 Fragment 목록에서 문서와 매칭하지 못한다.
- **제안**: `DisplayName = "Charges"` 로 통일한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`
- **교차 확인**: 호출부·소비자 검증을 위해 `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `Config/DefaultGame.ini` 를 함께 읽었다
- **이전 리뷰(`c486a5c7`) 대비 변동**:
  - 구 발견 1(`GetPrimaryAssetId()` 오버라이드가 AssetManager 등록 타입과 불일치)은 **해소됐다**. 오버라이드가 제거되어 `UPrimaryDataAsset` 기본 구현이 `WxItemDefinition:<자산명>` 을 돌려주고, `Config/DefaultGame.ini:50` 의 `PrimaryAssetType="WxItemDefinition"` 과 일치한다.
  - 구 발견 4(픽업 비주얼)는 **제안을 정정했다**. `ApplyPickupVisual` 전체를 데디케이티드 서버에서 막자는 이전 제안은 StaticMesh 가 곧 물리 바디·쿼리 콜리전이라는 사실을 놓쳤다 — 그대로 적용하면 물리 발사와 상호작용 판정이 함께 죽는다.
  - 클라 복제 순서(서브오브젝트 → 컴포넌트) 전제와 `check()` 기반 권위 가드는 이전 리뷰에서 반증 검증을 마쳤고 이번에도 코드가 그대로라 재검토하지 않았다.
- **미검토 / 한계**:
  - `.uasset` 실데이터(아이템 정의의 Fragment 조합, 보상 DataTable 의 `Quantity`, 픽업 BP 의 기본 메시)는 확인하지 않았다 — 발견 2·6 은 데이터 오설정 시의 위험이라 실 데이터 점검이 함께 필요하다.
  - 발견 3의 "서버에서 메시가 없으면 상호작용 불가"는 코드·계약 주석 대조로 도출했고 실제 데디케이티드 서버 구동으로 확인하지는 않았다. 픽업 BP 에 기본 메시가 박혀 있으면 발현하지 않을 수 있다.
  - 인벤토리 델리게이트 구독자(`Source/WxGame/MVVM/WxViewModel_*`)의 수명·해제 정확성은 WxGame 리뷰 범위로 넘겼다.

---
*문서 기준 커밋 `3d9e73c0` · 리뷰일 2026-09-04 · 소스 22파일 — `/module-review`로 갱신*
