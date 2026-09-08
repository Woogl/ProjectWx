# WxInventory — 코드 리뷰

> 모듈 경계와 코딩 규칙은 흠이 없다(WxCore 외 Wx 플러그인 의존 0, 저작권 헤더 22/22, 람다·`FORCEINLINE` 0, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward` 한 곳뿐이며 BP Function Library 라 정당, 헤더 정의는 템플릿·`GetInstanceDataType()` 예외에 사유 주석까지 붙어 있다). 남은 문제는 전부 복제·통지 계층과 인스턴스 수명주기에 몰려 있다. 이번 리뷰는 `README.md`·`.uplugin`·`*.Build.cs`·Public 헤더 전부를 읽고 `WxInventoryComponent.cpp`, `WxItemInstance.cpp`, `WxItemPickup.cpp`, `WxRewardLibrary.cpp`, `WxEquipmentComponent.cpp` 와 StateTree 태스크 2종을 정독했으며, 권한·통지 계약을 확인하려고 `Source/WxGame` 의 소비자(`WxItemUseComponent`, `WxViewModel_Inventory`, `WxCharacterBase`)와 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, 엔진의 `FGameplayEffectContext` 정의를 교차 조회했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 스택 변경 통지의 입자도가 경로마다 달라 획득 연출에 틀린 수량이 뜬다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:304`, `:320`, `:413`, `:619-628` / 소비자 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104-122`
- **범주**: 버그/정확성
- **문제**: 같은 델리게이트(`OnInventoryStackChanged`)가 경로마다 다른 단위로 발행된다. 추가 경로는 머지·분할된 슬롯 **하나마다** 1회(304·320행), 소비 경로는 배치 전체에 1회(413행, `-NumToConsume`), 클라이언트 복제 경로는 엔트리 하나마다 1회(83-90·101-112행)다. 구독자가 `NewCount` 만 읽으면 무해하지만 실제 구독자는 `Delta` 를 쓴다 — `UWxViewModel_Inventory::HandleStackChanged` 는 `Delta > 0` 일 때마다 획득 연출용 VM 을 새로 만들고 `AcquiredCount = Delta` 를 넣는다(110-117행, 대입은 116행). 결과적으로 마지막 조각의 델타만 화면에 남는다. 구체적으로: Stackable Fragment 가 없는(=`MaxStack` 1) 아이템을 보상으로 5개 주면 `Delta == 1` 통지가 5번 나가 토스트가 "x1" 로 뜨고, 97/99 인 슬롯에 5개를 더하면 "x2"(머지분) 뒤 "x3"(신규 슬롯분)이 떠 최종 표시가 x3 이 된다. 스탠드얼론·서버에서 그대로 재현되며 네트워크에서는 발행 횟수가 또 달라진다. 덤으로 `NotifyStackChangedFromList` 는 발행마다 626행에서 `GetTotalItemCountByDefinition` 으로 엔트리 전체를 훑으므로 대량 지급이 O(지급 슬롯 수 × 엔트리 수) 가 된다.
- **제안**: 발행 단위를 "변경 배치당 1회"로 통일한다. `AddItemDefinition` 이 머지·분할 델타를 누적해 함수 끝에서 한 번만 발행하면(소비 경로가 이미 그 형태다) 표시도 맞고 전체 재스캔도 지급당 1회로 줄어든다. 슬롯 단위 정보는 `OnInventorySlotChanged` 가 이미 담당한다.
- **확신도**: 높음

### 2. 🟡 FastArray 수신 콜백이 유효성 검사보다 먼저 `LastObservedCount` 를 갱신해 클라이언트 획득 델타가 영구 유실된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:83`, `:85`, `:88`, `:104-107`, `:621`
- **범주**: 버그/정확성
- **문제**: `PostReplicatedAdd` 는 83행에서 `Entry.LastObservedCount = Entry.StackCount;` 를 무조건 수행한 뒤 85행에서야 `Entry.Instance` 널 검사를 한다. 엔트리 도착 시점에 서브오브젝트 참조가 아직 NetGUID 미해결이면 통지는 건너뛰는데 관찰값만 최신으로 올라간다. 포인터가 나중에 해결돼도 FastArray 는 그 항목을 다시 dirty 로 보지 않고, 이후 `PostReplicatedChange` 가 와도 `Delta == 0` 이라 107행에서 걸러진다. 두 번째 경로도 같다 — 인스턴스는 해결됐지만 그 위의 `ItemDef` 가 아직 도착하지 않으면 88행이 `nullptr` 을 넘기고 `NotifyStackChangedFromList` 가 621행에서 조용히 반환하는데, 이때도 `LastObservedCount` 는 이미 최신이라 복구 기회가 없다(`HandleItemDefReplicated` 는 의도적으로 델타를 만들지 않고 "다시 읽어라" 신호만 보낸다). 목록 **표시** 는 `PostReplicatedReceive` → `OnInventoryContentsChanged`(120-127행)가 뒤늦게 따라잡지만, 발견 1 의 획득 연출은 해당 아이템에 대해 아예 뜨지 않는다. `PostReplicatedChange`(104-105행)도 같은 형태다.
- **제안**: `LastObservedCount` 갱신을 실제로 통지를 발행하는 분기 안(널 검사 + `ItemDef` 유효 확인 이후)으로 옮긴다. 그러면 미해결 슬롯은 다음 변경 때 누적 델타로 복구된다.
- **확신도**: 중간

### 3. 🟡 `Entries` 배열 참조를 가상 확장점·외부 브로드캐스트 너머까지 붙들고 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:136-151`, `:286-310`
- **범주**: 버그/정확성
- **문제**: 두 지점이 같은 형태다. (a) `FWxInventoryList::AddEntry` 는 136행 `Entries.AddDefaulted_GetRef()` 로 얻은 참조 `NewEntry` 를 들고 142-148행에서 가상 확장점 `Fragment->OnInstanceCreated(NewEntry.Instance)` 를 호출한 뒤 150행 `MarkItemDirty(NewEntry)`·151행 `return NewEntry.Instance` 를 이어간다. `OnInstanceCreated` 는 README 와 `Public/Items/WxItemFragment.h:40` 이 공식 확장점으로 광고하는 가상 함수인데, 그 안에서 같은 인벤토리에 아이템이 추가되면 `Entries` 재할당으로 `NewEntry` 가 댕글링된다. (b) `AddItemDefinition` 의 머지 루프는 286행에서 `const TArray<FWxInventoryEntry>& Entries` 를 잡고 루프 조건(287행)과 인덱싱(289·294·299행)에 계속 쓰면서, 루프 **안**의 303-304행에서 외부 구독자에게 브로드캐스트한다. 구독자가 같은 인벤토리를 변경하면 인덱스 의미와 배열 참조가 동시에 무효해진다(`Entries.Num()` 을 매 회전 재평가해 범위 밖 접근은 막히지만, 슬롯을 건너뛰거나 중복 머지할 수 있다). 현재 구독자인 뷰모델들은 읽기만 해서 발현되지 않지만, (b)는 임의의 구독자가 트리거할 수 있어 도달 경로가 넓다.
- **제안**: (a) `const int32 NewIndex = Entries.AddDefaulted();` 로 인덱스를 잡고 프래그먼트 호출 이후 `Entries[NewIndex]` 로 다시 접근한다. (b) 발견 1 의 배치 발행으로 옮기면 루프 안 브로드캐스트가 사라져 함께 해소된다.
- **확신도**: 중간

### 4. 🟡 장비 경로는 배선만 있는데 그 복제 비용은 전 캐릭터가 낸다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:14`, `:34-57`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h:179`, `:234`, `Source/WxGame/Character/WxCharacterBase.cpp:42`
- **범주**: 설계/구조
- **문제**: `UWxEquipmentComponent::EquipItem` 의 유일한 호출부인 `UWxInventoryComponent::EquipItemByDef` 를 부르는 곳이 저장소 전체에 0건이라(헤더 232행이 스스로 그렇게 적고 있다) `EquippedItemDef` 는 항상 널이고 `ApplyEquipEffects`/`RemoveActiveEquipEffects` 는 도달 불가다. 그런데 `WxCharacterBase.cpp:42` 가 모든 캐릭터(플레이어·적·미니언)에 이 컴포넌트를 기본 서브오브젝트로 만들고, 컴포넌트는 `SetIsReplicatedByDefault(true)`(14행)라 액터마다 쓰이지 않는 복제 등록을 하나씩 계속 소모한다. `UWxInventoryComponent::RemoveItemInstance`(179행)도 호출부 0건이다.
- **제안**: 장비 기능을 곧 쓸 계획이 없으면 `UWxEquipmentComponent`·`EquipItemByDef`·`RemoveItemInstance`·`UWxItemFragment_Equippable` 을 걷어낸다. 유지한다면 최소한 `WxCharacterBase` 의 무조건 생성만이라도 거둬 장비를 실제로 쓰는 캐릭터에만 부착한다.
- **확신도**: 높음

### 5. 🟢 `UseItemByDef` 가 차감 이후에 GE 를 적용해, 지속형 효과의 `SourceObject` 가 댕글링된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:538-541`, `:556`, `:561-564`
- **범주**: 버그/정확성
- **문제**: 539행이 `Context.AddSourceObject(SourceInstance)` 로 아이템 인스턴스를 GE 컨텍스트에 싣고, 556행에서 `ConsumeItemsByDefinition(ItemDef, 1)` 이 그 인스턴스의 엔트리를 통째로 제거한 뒤, 563행에서 GE 를 적용한다. 엔트리가 사라지면 `UWxItemInstance` 를 붙드는 `UPROPERTY` 참조가 남지 않는다(Outer 는 GC 를 막지 않으며, `UnregisterReplicatedInstance` 로 서브오브젝트 등록도 풀린다). 엔진의 `FGameplayEffectContext::SourceObject` 는 `TWeakObjectPtr<UObject>` 이므로(UE 5.8 `GameplayEffectTypes.h:443`), 즉발 GE 는 같은 프레임에 끝나 무사하지만 지속형·무한 GE 는 다음 GC 이후 `GetSourceObject()` 가 널이 된다. `UWxItemInstance` 헤더(`Public/Items/WxItemInstance.h:19`)가 SourceObject 를 "효과 측이 인스턴스별 데이터에 접근하는 진입점"으로 광고하고 있어, MMC/ExecCalc/GameplayCue 가 그것을 읽는 순간 조용히 널을 받는다. 지금은 소비 아이템이 충전형(에스트병) 하나뿐이라 이 경로(556행)를 타지 않지만, 지속 버프 물약을 추가하는 순간 발현된다.
- **제안**: 561-564행의 GE 적용을 548행의 차감보다 **앞으로** 옮긴다(가용성·Spec 검증은 이미 그 앞에서 끝났다). 차감 후 적용을 유지해야 한다면 적용이 끝날 때까지 인스턴스를 강참조로 붙든다.
- **확신도**: 중간

### 6. 🟢 `MaxStack` 이 0 이하면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:279`, `:313-331`
- **범주**: 성능/안전
- **문제**: 315행 `ChunkCount = FMath::Min(MaxStack, Remaining)` 이 0 이면 326행 `Remaining -= ChunkCount` 가 진전을 못 만들어, while 루프가 매 회전마다 새 엔트리와 `UWxItemInstance` 를 만들며 영원히 돈다(행 + OOM). `UWxItemFragment_Stackable::MaxStack` 의 `ClampMin = "1"`(`Public/Items/WxItemFragment.h:109`)은 디테일 패널 입력만 막을 뿐, 클램프가 붙기 전에 저장된 값이나 툴셋·스크립트로 직접 쓰인 값은 막지 못한다.
- **제안**: 279행을 `const int32 MaxStack = FMath::Max(1, Stackable ? Stackable->MaxStack : 1);` 로 한 번 방어한다.
- **확신도**: 중간

### 7. 🟢 Stackable 과 Charges 를 함께 붙이면 스택 전체가 충전량 하나를 공유한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:284-311`, `:548-555`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp:11-19`
- **범주**: 설계/구조
- **문제**: 충전량은 `UWxItemInstance` 단위인데 Stackable 은 여러 개를 인스턴스 하나로 머지한다. 두 Fragment 를 함께 부착한 아이템을 3개 획득하면, 첫 획득만 `OnInstanceCreated` 로 `MaxCharges` 를 채우고(`WxItemFragment.cpp:17`) 이후 획득은 머지 경로(284-311행)라 인스턴스를 만들지 않아 충전량이 늘지 않는다. `UseItemByDef` 는 Charges 가 있으면 스택을 건드리지 않고 그 하나의 충전량만 1 줄이므로(548-555행), 3개를 들고도 총 `MaxCharges` 회만 쓸 수 있다. `UWxItemDefinition` 에 `IsDataValid` 가 없어 저작 시 아무 경고도 없다. 두 축을 직교로 설계한 것 자체는 Fragment 컴포지션의 의도이나, 이 조합만은 실질적으로 성립하지 않는다.
- **제안**: `UWxItemDefinition::IsDataValid` 를 추가해 Stackable + Charges 동시 부착을 에러로 잡는다(주석에 "직교"라 적힌 Usable + Charges 와 달리 이쪽은 무효 조합이라는 점을 `WxItemFragment.h` Charges 주석에도 남긴다).
- **확신도**: 중간

### 8. 🟢 `GrantReward` 가 `World` 만 검사 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:43`, `:75`
- **범주**: 성능/안전
- **문제**: `SourceActor`·`RewardRow`·`Row`·`ItemDef`·`ItemActorClass`·`SpawnedPickup` 을 모두 방어적으로 검사하는 함수에서 `UWorld* World = SourceActor->GetWorld();`(43행)만 검사 없이 75행 `World->SpawnActorDeferred` 로 넘어간다. 모듈에서 유일하게 BP 에 노출된 진입점이라 정리 중인 액터·CDO 가 들어올 여지가 있고, 나머지 방어와 일관되지도 않다.
- **제안**: 43행 뒤에 `if (!World) { return; }` 를 붙인다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`
- **훑은 파일**: `Plugins/WxInventory/README.md`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `.../Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `.../Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `.../Private/Items/WxItemDefinition.cpp`, `.../Private/Items/WxItemFragment.cpp`, `.../Private/Items/WxRewardTableRow.cpp`, `.../Private/WxInventoryModule.cpp`, 나머지 Public 헤더 전부. 계약 교차 확인으로 `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`
- **미검토 / 한계**:
  - StateTree 태스크 2종이 대상 인벤토리를 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 고정하는 점(`WxStateTreeTask_GiveRewards.cpp:40`, `WxStateTreeTask_RefillItemCharges.cpp:36`)은 발견으로 올리지 않았다. `Source/WxGame/Character/WxEnemyCharacter.cpp:177` 이 "처치자를 가리지 않고 항상 0번 플레이어에게 지급하는 것이 기존 정책"이라고 명시하고 있어 모듈 고유 결함이 아니다. 다만 WxWorld 의 장치 태스크들은 `AWxDevice::GetInteractingCharacter()` 로 당사자를 제대로 집으므로, 멀티 정책을 정하는 시점에 이 두 태스크의 인스턴스 데이터에 바인딩 가능한 대상 액터를 노출하는 방향으로 함께 봐야 한다(WxInventory 는 WxWorld 를 참조할 수 없으니 바인딩 입력이 유일한 해법이다).
  - "Pawn → PlayerController → `FindComponentByClass<UWxInventoryComponent>`" 관용구가 5곳에 복제돼 있다(`WxItemPickup.cpp:88-90`, `WxRewardLibrary.cpp:39-41`, `WxStateTreeTask_RefillItemCharges.cpp:36-37`, `WxItemInstance.cpp:85-86`·`:95-96`, 모듈 밖으로는 `Source/WxGame/Inventory/WxItemUseComponent.cpp:16-18`·`:86-88`). 공용 정적 헬퍼가 과거에 의도적으로 제거됐을 가능성이 있어 발견으로 올리지 않았고, 커밋 이력까지 확인하지는 못했다.
  - `PreReplicatedRemove`(44-70행)가 한 번의 번치로 같은 `ItemDef` 의 여러 슬롯을 지울 때, 첫 통지의 `NewCount` 가 아직 0 으로 내려가지 않은 나머지 슬롯을 함께 세어 과대 계상된다. 마지막 통지와 `PostReplicatedReceive` 로 수렴하므로 발견으로 올리지 않았다.
  - 발견 2 는 실제 델타 패킷 도착 순서에 달린 문제라 PIE 리슨/데디 실측 없이 코드 경로 추론에 근거한다. 이번 리뷰에서 빌드·에디터 실행은 하지 않았다.
  - 아이템 정의·보상 DataTable 등 데이터 에셋의 실제 값(`MaxStack`, `ItemActorClass` 설정 여부, Usable Effect 의 지속 정책 등)과 BP/WBP 내부 구조는 열어보지 않았다. 발견 5·7 의 발현 여부는 그 데이터에 달려 있다.
  - 모듈에 자동화 테스트가 없다(WxWorld 의 `WxDeviceTests.cpp` 같은 대응물 부재). 수량·슬롯 경계 로직이 순수 함수에 가까워 테스트 가치가 높은 편이라는 점만 적어 둔다.

---
*문서 기준 커밋 `262e4cca` · 리뷰일 2026-09-08 · 소스 22파일 — `/module-review`로 갱신*
