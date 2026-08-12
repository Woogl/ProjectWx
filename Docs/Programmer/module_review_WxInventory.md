# WxInventory — 코드 리뷰

> 2,356줄 22파일의 작은 도메인 플러그인이며, 규칙 준수(복사권 헤더·Wx prefix·람다 부재·BlueprintCallable 한정)와 FastArray 사용법 자체는 정확하다. 다만 아이템 인스턴스 서브오브젝트 복제가 소유 액터 설정 누락으로 실제로는 동작하지 않는다. 이번 리뷰는 `*.Build.cs`/`.uplugin`, 전체 헤더, 그리고 `WxInventoryManagerComponent.cpp`·`WxItemPickup.cpp`·`WxRewardLibrary.cpp`·`WxEquipmentComponent.cpp` 등 핵심 cpp 전량을 읽었고, 복제 순서·서브오브젝트 등록 동작은 UE 5.8 엔진 소스로 대조 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🔴 `UWxItemInstance` 서브오브젝트가 클라이언트로 전혀 복제되지 않는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:225`, `:242-253`, `:692-706`
- **범주**: 버그/정확성
- **문제**: 컴포넌트는 `bReplicateUsingRegisteredSubObjectList = true`를 켜고 `AddReplicatedSubObject`로 인스턴스를 등록하지만, 엔진은 **소유 액터**의 플래그로 이 등록 목록의 사용 여부를 가른다. `UActorChannel::DoSubObjectReplication`(`Engine/Source/Runtime/Engine/Private/DataChannel.cpp:3989`)은 `if (Actor->IsUsingRegisteredSubObjectList())`일 때만 `ReplicateRegisteredSubObjects`로 컴포넌트의 등록 목록을 순회하고, 아니면 레거시 `AActor::ReplicateSubobjects`(`Private/ActorReplication.cpp:592`)로 빠지는데 그 경로는 `UActorComponent::ReplicateSubobjects`의 기본 구현(항상 `false` 반환)만 부를 뿐 등록 목록을 보지 않는다.
  액터 측 플래그는 `GDefaultUseSubObjectReplicationList`에서 오고 UE 5.8 기본값은 `false`(`Private/Components/ActorComponent.cpp:115`)다. 프로젝트 `Config/` 어디에도 `net.SubObjects.DefaultUseSubObjectReplicationList=1`이 없고, `Source/WxGame/Controller/WxPlayerController.cpp:7-12`의 생성자도 플래그를 켜지 않는다.
  결과적으로 리슨/데디케이티드 서버의 원격 클라이언트에서는 `UWxItemInstance`가 만들어지지 않는다. FastArray 엔트리의 `Instance` NetGUID가 영영 해석되지 않으므로 `PostReplicatedAdd`/`PostReplicatedChange`의 `if (Entry.Instance)` 가드(`:94`, `:116`)에 전부 걸려 슬롯·합계 통지가 한 건도 발행되지 않고, `UWxItemInstance::CurrentCharges`의 `OnRep_CurrentCharges`도 절대 호출되지 않는다(인벤토리 UI·에스트병 충전 표시가 클라에서 백지). 스탠드얼론/PIE 단일에서는 복제 자체가 없어 서버 경로 직접 통지로 정상 동작하므로 지금까지 드러나지 않았을 뿐인 잠복 결함이다.
- **제안**: `AWxPlayerController` 생성자에서 `bReplicateUsingRegisteredSubObjectList = true`를 설정하거나(인벤토리만 고치는 최소 변경), `Config/DefaultEngine.ini`의 `[SystemSettings]`에 `net.SubObjects.DefaultUseSubObjectReplicationList=1`을 넣어 프로젝트 전역으로 켠다(엔진이 Iris 사용 시 권장하는 방식). 어느 쪽이든 도메인 플러그인이 소유 액터 설정에 암묵적으로 의존하는 상태이므로, `UWxInventoryManagerComponent::ReadyForReplication`에서 `GetOwner()->IsUsingRegisteredSubObjectList()`를 `ensureMsgf`로 확인해 배선 누락이 조용히 지나가지 않게 한다.
- **확신도**: 높음

### 2. 🟡 `AddItemDefinition` 머지 루프가 델리게이트 방송을 사이에 두고 인덱스를 재사용한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:291-315`
- **범주**: 버그/정확성
- **문제**: `const TArray<FWxInventoryEntry>& Entries = InventoryList.GetEntries();`로 내부 배열 참조를 캐시한 뒤 루프 안에서 `NotifySlotChangedFromList`/`NotifyStackChangedFromList`(`:308-309`)로 외부 구독자에게 방송하고, 다음 반복에서 다시 `Entries[EntryIndex]`와 `InventoryList.AddToEntryStack(EntryIndex, ...)`를 인덱스로 접근한다. 구독자가 방송 처리 중에 인벤토리를 변경(예: 획득 즉시 자동 소비, 퀘스트 완료 보상 회수)하면 `ConsumeByDefinition`의 `It.RemoveCurrent()`로 엔트리가 앞당겨져 `EntryIndex`가 다른 슬롯을 가리키고, 잔여분이 엉뚱한 슬롯에 더해진다. 현재 구독자(`Source/WxGame/MVVM/WxViewModel_Inventory.cpp:98`의 `HandleStackChanged`, `WxViewModel_Item`)는 읽기만 하므로 지금 당장 재현되지는 않는 잠복 위험이다. 같은 함수의 `:318-336` 신규 엔트리 루프도 `AddEntry` → 방송 → 다음 `AddEntry` 순서라 동일한 재진입 창을 갖는다.
- **제안**: 변경(머지·추가)을 먼저 전부 적용해 `FWxInventoryChangeResult` 목록으로 모아두고, 루프가 끝난 뒤 그 목록으로 통지를 일괄 발행한다 — `ConsumeItemsByDefinition`(`:406-419`)이 이미 쓰는 "변경 후 통지" 형태와 맞춘다.
- **확신도**: 중간

### 3. 🟡 한 보상 Row 의 픽업들이 완전히 같은 위치·같은 속도로 스폰되어 물리적으로 겹친다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:40-79`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:31-37`, `:59-69`
- **범주**: 버그/정확성
- **문제**: `GrantReward`의 보상 루프는 항목마다 동일한 `SpawnTransform`으로 `SpawnActorDeferred`하고(`:70`) 동일한 `LaunchVelocity`로 `LaunchInDirection`한다(`:79`). 픽업의 루트 메시는 `ECC_WorldDynamic` 오브젝트 타입에 `SetCollisionResponseToAllChannels(ECR_Block)`(`WxItemPickup.cpp:32-33`)이라 픽업끼리 서로 Block하고, `LaunchInDirection`은 `SetSimulatePhysics(true)`를 건다. `FWxRewardTableRow`는 픽업 보상을 최대 5개까지 담을 수 있으므로(`Public/Items/WxRewardTableRow.h:42-55`) 상자 하나가 픽업 2개 이상을 드랍하면 완전히 겹친 상태로 스폰되어 디페네트레이션 임펄스로 튀어 나간다.
- **제안**: 루프 인덱스에 따라 스폰 위치를 원형/부채꼴로 분산시키거나, `LaunchVelocity`에 항목별 수평 산포를 더한다. 픽업끼리는 서로 무시하도록 별도 오브젝트 채널을 주는 방법도 있다.
- **확신도**: 중간

### 4. 🟢 제거된 `UWxItemInstance` 가 세션 내내 회수되지 않는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:154-165`, `:175-207`, `:700-706`
- **범주**: 성능/안전
- **문제**: 인스턴스는 `NewObject<UWxItemInstance>(OwningActor)`로 PlayerController를 Outer 삼아 만들어지고(`:137`), 소비·제거 시 엔트리에서만 빠진다. UE GC는 Outer를 강한 참조로 취급하므로(`GarbageCollectionSchema.h`의 `Outer`/`ClassOuter` 토큰) 엔트리에서 빠진 인스턴스도 PlayerController 수명 내내 살아 있다. `MarkAsGarbage()` 호출이나 `DestroyReplicatedSubObjectOnRemotePeers` 호출이 없어 서버·클라 양쪽에서 소비량에 비례해 UObject가 단조 증가한다. 개당 크기가 작아 당장 문제는 아니지만 장시간 세션에서 GC 순회 대상이 계속 늘어난다.
- **제안**: `RemoveEntry`/`ConsumeByDefinition`으로 슬롯이 사라질 때 해당 인스턴스에 `MarkAsGarbage()`를 호출하고, 복제 해제는 `RemoveReplicatedSubObject` 대신 `DestroyReplicatedSubObjectOnRemotePeers`로 바꿔 클라 사본도 함께 정리한다.
- **확신도**: 중간

### 5. 🟢 `GetPrimaryAssetId` 가 등록되지 않은 PrimaryAssetType 을 반환한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp:12-15`
- **범주**: 설계/구조
- **문제**: `FPrimaryAssetId(TEXT("WxItem"), GetFName())`을 반환하지만 `Config/DefaultGame.ini:46-50`의 `PrimaryAssetTypesToScan`에는 `Map`/`PrimaryAssetLabel`/`GameFeatureData`/`WxExperienceDefinition`/`WxExperienceActionSet`만 있고 `WxItem` 항목이 없으며, 커스텀 `UAssetManager` 서브클래스도 없다. 따라서 AssetManager 가 아이템 정의를 프라이머리 애셋으로 등록하지 않아 이 오버라이드는 사실상 무효다 — `GetPrimaryAssetPath`로 역해석이 안 되고, 아이템 정의에 쿠킹 규칙(`CookRule`)·청크·번들 상태를 붙일 수단이 없다. 나중에 `LoadPrimaryAsset("WxItem", ...)`를 시도하는 코드가 조용히 실패하는 함정이 된다.
- **제안**: `/Game/Item` 을 스캔 대상으로 하는 `WxItem` 타입을 `PrimaryAssetTypesToScan`에 추가하거나, 프라이머리 애셋으로 관리할 의도가 없다면 오버라이드를 제거해 `UPrimaryDataAsset`의 기본 동작을 쓴다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryStateTreeNodes.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryStateTreeNodes.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxRewardStateTreeNodes.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`
- **미검토 / 한계**:
  - 규칙 위반은 기계적으로 전수 확인했고 위반이 없다 — 소스 22파일 전부 첫 줄 저작권 표기 일치, 람다 0건, `BlueprintCallable`은 `UWxRewardLibrary::GrantReward`(BP Function Library) 한 곳뿐, `WxCore` 외 Wx 플러그인 의존 없음, `Wx` prefix 누락 없음, override의 `Super::` 호출 누락 없음. 헤더 인라인 정의는 `FindFragmentByClass<T>` 템플릿과 StateTree `GetInstanceDataType()` 두 종뿐이며 코드 주석에 규칙 6 예외 사유가 명시돼 있어 발견으로 올리지 않았다.
  - 장비 경로(`EquipItemByDef` → `UWxEquipmentComponent::EquipItem`)는 호출부가 0건이라 코드 자체가 미배선 상태다(헤더에 명시됨). 이 상태에서 나올 수 있는 문제(해제 시 `BroadcastEquipVisual(nullptr, NAME_None)`이 "외형 유지"로 해석돼 무장 해제가 불가능한 점, 늦게 relevant 해진 클라의 초기 RepNotify 유실)는 이미 `WxEquipmentComponent.h:28-31` 주석에 기록돼 있어 중복 지적하지 않았다.
  - BP/WBP 내부(`BP_ItemPickup`, `WBP_ItemSlot`, `DA_Potion` 등 데이터 자산의 Fragment 실제 구성)는 범위 밖이라 확인하지 않았다 — Fragment 조합 규약(Charges + Usable 동시 부착 등)이 실제 자산에서 지켜지는지는 미검증이다.
  - 🔴 1번의 클라이언트 증상은 엔진 소스 대조로 도출했고 실제 네트워크 세션 재현은 하지 않았다(빌드·에디터 실행 금지 조건).

---
*문서 기준 커밋 `ebe6cffd` · 리뷰일 2026-08-12 · 소스 22파일 — `/module-review`로 갱신*
