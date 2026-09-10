# WxInventory — 코드 리뷰

> 모듈 경계와 코딩 규칙은 흠이 없다(WxCore 외 Wx 플러그인 의존 0, 저작권 헤더 20/20, 람다·`FORCEINLINE`·델리게이트 바인딩 0, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward` 한 곳뿐이며 BP Function Library 라 정당, 헤더 정의는 템플릿·StateTree `GetInstanceDataType()` 예외에 사유 주석까지 붙어 있다). 지난 리뷰(`262e4cca`) 이후 미사용 장비 경로가 실제로 제거되어 갓 클래스 성향이 줄었고, 남은 문제는 전부 복제 통지 계층과 인스턴스 수명주기에 몰려 있다. 이번 리뷰는 `README.md`·`.uplugin`·`*.Build.cs`·Public 헤더 전부를 읽고 `WxInventoryComponent.cpp`, `WxItemInstance.cpp`, `WxItemPickup.cpp`, `WxRewardLibrary.cpp` 와 StateTree 태스크 2종을 정독했으며, 권한·통지 계약 검증을 위해 `Source/WxGame` 소비자(`WxItemUseComponent`, `WxViewModel_Inventory`)와 `WxAbility_Interact`, 그리고 엔진의 `FastArraySerializer.h`·`GameplayEffectTypes.h` 를 교차 조회했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 스택 변경 통지의 입자도가 경로마다 달라 획득 연출에 틀린 수량이 뜬다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:303`, `:319`, `:412`, `:590-598` / 소비자 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104-121`
- **범주**: 버그/정확성
- **문제**: 같은 델리게이트(`OnInventoryStackChanged`)가 경로마다 다른 단위로 발행된다. 추가 경로는 머지·분할된 슬롯 **하나마다** 1회(303·319행), 소비 경로는 배치 전체에 1회(412행, `-NumToConsume`), 클라이언트 복제 경로는 엔트리 하나마다 1회(87·109행)다. 구독자가 `NewCount` 만 읽으면 무해하지만 실제 구독자는 `Delta` 를 쓴다 — `UWxViewModel_Inventory::HandleStackChanged` 는 `Delta > 0` 마다 획득 연출용 VM 을 새로 만들어 `AcquiredCount = Delta` 를 넣고 `LastAcquiredItem` 에 덮어쓴다(`WxViewModel_Inventory.cpp:110-119`). 결과적으로 마지막 조각의 델타만 화면에 남는다. 구체적으로: Stackable Fragment 가 없는(=`MaxStack` 1) 아이템 5개를 보상으로 주면 `Delta == 1` 통지가 5번 나가 토스트가 "x1" 로 뜨고, 97/99 슬롯에 5개를 더하면 "x2"(머지분) 뒤 "x3"(신규 슬롯분)이 떠 최종 표시가 x3 이 된다. 스탠드얼론·서버에서 그대로 재현된다. 덤으로 발행마다 `NotifyStackChangedFromList` 가 597행에서 엔트리 전체를 훑고 구독자가 `RefreshAllItems()`(`WxViewModel_Inventory.cpp:121`, 내부는 기존 VM 재사용을 위한 중첩 루프)를 다시 돌리므로, 대량 지급 1회가 슬롯 수만큼의 전체 재스캔·VM 생성으로 증폭된다.
- **제안**: 발행 단위를 "변경 배치당 1회"로 통일한다. `AddItemDefinition` 이 머지·분할 델타를 누적해 함수 끝에서 한 번만 발행하면(소비 경로가 이미 그 형태다) 표시도 맞고 전체 재스캔도 지급당 1회로 줄어든다. 슬롯 단위 정보는 `OnInventorySlotChanged` 가 이미 담당한다.
- **확신도**: 높음

### 2. 🟡 FastArray 수신 콜백이 유효성 검사보다 먼저 `LastObservedCount` 를 갱신해, 엔진이 주는 복구 콜백을 스스로 버린다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:82`, `:84`, `:87`, `:103-106`, `:592`
- **범주**: 버그/정확성
- **문제**: `PostReplicatedAdd` 는 82행에서 `Entry.LastObservedCount = Entry.StackCount;` 를 무조건 수행한 뒤 84행에서야 `Entry.Instance` 널 검사를 한다. 엔트리 도착 시점에 서브오브젝트 참조가 아직 NetGUID 미해결이면 통지는 건너뛰는데 관찰값만 최신으로 올라간다. 엔진은 GUID 가 나중에 매핑될 때 그 항목에 대해 `PostReplicatedChange` 를 다시 호출해 주지만(UE 5.8 `FastArraySerializer.h:1362`·`:1385`, `bUpdateUnmappedObjects` 경로), 그때는 이미 `Delta == Entry.StackCount - Entry.LastObservedCount == 0` 이라 106행 `Delta != 0` 가드가 통째로 걸러낸다 — 즉 복구 기회가 코드 자신에 의해 소거된다. 두 번째 경로도 같다: 인스턴스는 해결됐지만 그 위의 `ItemDef` 가 아직 도착하지 않으면 87행이 `nullptr` 을 넘기고 `NotifyStackChangedFromList` 가 592행에서 조용히 반환하는데, 이때도 관찰값은 최신이다(`OnRep_ItemDef` 는 의도적으로 델타를 만들지 않고 "다시 읽어라" 신호만 보낸다). 목록 **표시** 는 `PostReplicatedReceive` → `OnInventoryContentsChanged`(119-126행)가 뒤늦게 따라잡으므로 발현 범위는 발견 1 의 획득 연출이 해당 아이템에 대해 아예 뜨지 않는 것이다. `PostReplicatedChange`(103-104행)도 같은 형태다.
- **제안**: `LastObservedCount` 갱신을 실제로 통지를 발행하는 분기 안(널 검사 + `ItemDef` 유효 확인 이후)으로 옮긴다. 그러면 미해결 슬롯은 GUID 매핑 시 재호출되는 `PostReplicatedChange` 에서 누적 델타로 복구된다.
- **확신도**: 중간 (엔진의 재호출은 소스로 확인했으나, 인스턴스가 실제로 미해결 상태로 도착하는 빈도는 번치 순서에 달려 있고 PIE 실측은 하지 않았다)

### 3. 🟡 `Entries` 배열 참조·인덱스를 가상 확장점·외부 브로드캐스트 너머까지 붙들고 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:135-150`, `:285-309`
- **범주**: 버그/정확성
- **문제**: 두 지점이 같은 형태다. (a) `FWxInventoryList::AddEntry` 는 135행 `Entries.AddDefaulted_GetRef()` 로 얻은 참조 `NewEntry` 를 들고 141-147행에서 가상 확장점 `Fragment->OnInstanceCreated(NewEntry.Instance)` 를 호출한 뒤 149행 `MarkItemDirty(NewEntry)`·150행 `return NewEntry.Instance` 를 이어간다. `OnInstanceCreated` 는 README 와 `Public/Items/WxItemFragment.h:40` 이 공식 확장점으로 광고하는 가상 함수인데, 그 안에서 같은 인벤토리에 아이템이 추가되면 `Entries` 재할당으로 `NewEntry` 가 댕글링된다. (b) `AddItemDefinition` 의 머지 루프는 285행에서 `const TArray<FWxInventoryEntry>& Entries` 를 잡고 루프 조건(286행)과 인덱싱(288·293·298·299행)에 계속 쓰면서, 루프 **안**의 302-303행에서 외부 구독자에게 브로드캐스트한다. 구독자가 같은 인벤토리를 변경하면 인덱스 의미와 배열 참조가 동시에 무효해진다(`Entries.Num()` 을 매 회전 재평가해 범위 밖 접근은 막히지만, 슬롯을 건너뛰거나 중복 머지할 수 있다). 현재 구독자인 뷰모델들은 읽기만 해서 발현되지 않지만, (b)는 임의의 구독자가 트리거할 수 있어 도달 경로가 넓다.
- **제안**: (a) `const int32 NewIndex = Entries.AddDefaulted();` 로 인덱스를 잡고 프래그먼트 호출 이후 `Entries[NewIndex]` 로 다시 접근한다. (b) 발견 1 의 배치 발행으로 옮기면 루프 안 브로드캐스트가 사라져 함께 해소된다.
- **확신도**: 중간

### 4. 🟢 `UseItemByDef` 가 차감 이후에 GE 를 적용해, 지속형 효과의 `SourceObject` 가 댕글링된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:537-540`, `:555`, `:560-563`
- **범주**: 버그/정확성
- **문제**: 538행이 `Context.AddSourceObject(SourceInstance)` 로 아이템 인스턴스를 GE 컨텍스트에 싣고, 555행에서 `ConsumeItemsByDefinition(ItemDef, 1)` 이 그 인스턴스의 엔트리를 통째로 제거한 뒤, 562행에서 GE 를 적용한다. 엔트리가 사라지면 `UWxItemInstance` 를 붙드는 `UPROPERTY` 참조가 남지 않는다(Outer 는 GC 를 막지 않으며, `UnregisterReplicatedInstance` 로 서브오브젝트 등록도 풀린다). 엔진의 `FGameplayEffectContext::SourceObject` 는 `TWeakObjectPtr<UObject>` 이므로(UE 5.8 `GameplayEffectTypes.h:443`) 즉발 GE 는 같은 프레임에 끝나 무사하지만 지속형·무한 GE 는 다음 GC 이후 `GetSourceObject()` 가 널이 된다. `Public/Items/WxItemInstance.h:19` 가 SourceObject 를 "효과 측이 인스턴스별 데이터에 접근하는 진입점"으로 광고하고 있어, MMC/ExecCalc/GameplayCue 가 그것을 읽는 순간 조용히 널을 받는다. 저장소 전체에 `GetSourceObject` 호출부가 아직 0건이고 소비 아이템도 충전형(에스트병) 하나뿐이라 이 경로(555행)를 타지 않지만, 지속 버프 물약을 추가하는 순간 발현된다.
- **제안**: 560-563행의 GE 적용을 547행의 차감 분기보다 **앞으로** 옮긴다(가용성·Spec 검증은 이미 그 앞에서 끝났다). 차감 후 적용을 유지해야 한다면 적용이 끝날 때까지 인스턴스를 강참조로 붙든다.
- **확신도**: 중간

### 5. 🟢 `MaxStack` 이 0 이하면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:278`, `:312-330`
- **범주**: 성능/안전
- **문제**: 314행 `ChunkCount = FMath::Min(MaxStack, Remaining)` 이 0 이면 325행 `Remaining -= ChunkCount` 가 진전을 못 만들어, while 루프가 매 회전마다 새 엔트리와 `UWxItemInstance` 를 만들며 영원히 돈다(행 + OOM). `UWxItemFragment_Stackable::MaxStack` 의 `ClampMin = "1"`(`Public/Items/WxItemFragment.h:98`)은 디테일 패널 입력만 막을 뿐, 클램프가 붙기 전에 저장된 값이나 툴셋·스크립트로 직접 쓰인 값은 막지 못한다.
- **제안**: 278행을 `const int32 MaxStack = FMath::Max(1, Stackable ? Stackable->MaxStack : 1);` 로 한 번 방어한다.
- **확신도**: 중간

### 6. 🟢 Stackable 과 Charges 를 함께 붙이면 스택 전체가 충전량 하나를 공유한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:283-310`, `:547-554`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp:11-19`
- **범주**: 설계/구조
- **문제**: 충전량은 `UWxItemInstance` 단위인데 Stackable 은 여러 개를 인스턴스 하나로 머지한다. 두 Fragment 를 함께 부착한 아이템을 3개 획득하면, 첫 획득만 `OnInstanceCreated` 로 `MaxCharges` 를 채우고(`WxItemFragment.cpp:17`) 이후 획득은 머지 경로(283-310행)라 인스턴스를 만들지 않아 충전량이 늘지 않는다. `UseItemByDef` 는 Charges 가 있으면 스택을 건드리지 않고 그 하나의 충전량만 1 줄이므로(547-554행), 3개를 들고도 총 `MaxCharges` 회만 쓸 수 있다. `UWxItemDefinition` 에 `IsDataValid` 가 없어 저작 시 아무 경고도 없다. 두 축을 직교로 설계한 것 자체는 Fragment 컴포지션의 의도이나, 이 조합만은 실질적으로 성립하지 않는다.
- **제안**: `UWxItemDefinition::IsDataValid` 를 추가해 Stackable + Charges 동시 부착을 에러로 잡는다(주석에 "직교"라 적힌 Usable + Charges 와 달리 이쪽은 무효 조합이라는 점을 `WxItemFragment.h` Charges 주석에도 남긴다).
- **확신도**: 중간

### 7. 🟢 `GrantReward` 가 `World` 만 검사 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:43`, `:75`
- **범주**: 성능/안전
- **문제**: `SourceActor`·`RewardRow`·`Row`·`ItemDef`·`ItemActorClass`·`SpawnedPickup` 을 모두 방어적으로 검사하는 함수에서 `UWorld* World = SourceActor->GetWorld();`(43행)만 검사 없이 75행 `World->SpawnActorDeferred` 로 넘어간다. 모듈에서 유일하게 BP 에 노출된 진입점이라 정리 중인 액터·CDO 가 들어올 여지가 있고, 나머지 방어와 일관되지도 않다.
- **제안**: 43행 뒤에 `if (!World) { return; }` 를 붙인다.
- **확신도**: 중간

### 8. 🟢 `RemoveItemInstance` 는 호출부 0건인 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h:175-179`, `Private/Inventory/WxInventoryComponent.cpp:352-384`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체에서 호출부가 0건이고, 헤더 177행이 스스로 "미구현: 현재 호출부가 0건이다"라고 적고 있다. 지난 리뷰에서 같은 지적을 받은 장비 경로(`UWxEquipmentComponent`·`EquipItemByDef`·`UWxItemFragment_Equippable`)는 `a5ff3177` 로 실제 제거됐는데 이 함수만 남았다. 33행 분량이 "슬롯 통째 제거 + 등록 해제 + 통지" 순서를 별도로 재구현하고 있어(소비 경로와 통지 순서가 미묘하게 다르다) 유지되는 동안 계속 두 갈래 진실을 만든다.
- **제안**: 장비·버리기 기능을 곧 쓸 계획이 없으면 함수를 걷어낸다. 남긴다면 헤더의 "미구현" 문구 대신 어떤 기능이 이것을 쓸 예정인지 적어 둔다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`
- **훑은 파일**: `Plugins/WxInventory/README.md`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `.../Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `.../Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `.../Public/Inventory/WxStateTreeTask_GiveRewards.h`, `.../Public/Inventory/WxStateTreeTask_RefillItemCharges.h`, `.../Private/Items/WxItemDefinition.cpp`, `.../Public/Items/WxItemDefinition.h`, `.../Private/Items/WxItemFragment.cpp`, `.../Private/Items/WxRewardTableRow.cpp`, `.../Public/Items/WxRewardTableRow.h`, `.../Public/Items/WxItemPickup.h`, `.../Public/WxRewardLibrary.h`, `.../Private/WxInventoryModule.cpp`, `.../Public/WxInventoryModule.h`. 계약 교차 확인으로 `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`
- **미검토 / 한계**:
  - StateTree 태스크 2종이 대상 인벤토리를 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 고정하는 점(`WxStateTreeTask_GiveRewards.cpp:40`, `WxStateTreeTask_RefillItemCharges.cpp:36`)은 이번에도 발견으로 올리지 않았다. `Source/WxGame/Character/WxEnemyCharacter.cpp:184` 가 "처치자를 가리지 않고 항상 0번 플레이어에게 지급하는 것이 기존 정책"이라고 여전히 명시하고 있어 모듈 고유 결함이 아니다. 다만 픽업 경로(`WxItemPickup.cpp:88-90`)는 당사자(Interactor)를 제대로 집으므로 같은 모듈 안에 두 정책이 공존한다 — 멀티 정책을 정하는 시점에 두 태스크의 인스턴스 데이터에 바인딩 가능한 대상 액터를 노출하는 방향으로 함께 봐야 한다(WxInventory 는 WxWorld 를 참조할 수 없으니 바인딩 입력이 유일한 해법이다).
  - "Pawn → PlayerController → `FindComponentByClass<UWxInventoryComponent>`" 관용구가 5곳에 복제돼 있다(`WxItemPickup.cpp:88-90`, `WxRewardLibrary.cpp:39-41`, `WxStateTreeTask_RefillItemCharges.cpp:36-37`, `WxItemInstance.cpp:85-86`·`:95-96`, 모듈 밖으로는 `Source/WxGame/Inventory/WxItemUseComponent.cpp:16-18`·`:86-88`). 공용 정적 헬퍼가 과거에 의도적으로 제거됐을 가능성이 있어 발견으로 올리지 않았고, 커밋 이력까지 확인하지는 못했다.
  - `AWxItemPickup::OnInteracted`(`WxItemPickup.cpp:74-102`)와 `SetItemDef`(`:55-60`)는 주석으로만 서버 권위를 선언하고 `HasAuthority()` 게이트가 없다. 현재 유일한 호출 경로가 `UWxAbility_Interact`(`NetExecutionPolicy = ServerOnly`, `WxAbility_Interact.cpp:18`·`:49`)와 권위 게이트된 `GrantReward` 뿐이라 발견으로 올리지 않았다. 다만 클라이언트 경로가 생기면 `AddItemDefinition` 의 `check(HasAuthority())`(`WxInventoryComponent.cpp:275`)에서 셰이핑 빌드까지 크래시한다는 점만 적어 둔다.
  - `PreReplicatedRemove`(43-69행)가 한 번의 번치로 같은 `ItemDef` 의 여러 슬롯을 지울 때, 첫 통지의 `NewCount` 가 아직 0 으로 내려가지 않은 나머지 슬롯을 함께 세어 과대 계상된다. 마지막 통지와 `PostReplicatedReceive` 로 수렴하므로 발견으로 올리지 않았다(엔진이 실제 제거를 Add/Change 콜백 이후로 미루는 것은 `FastArraySerializer.h:1148`·`:1185-1196` 으로 확인했다).
  - `AWxItemPickup::ApplyPickupVisual` 이 `SpawnActorDeferred` → `FinishSpawning` 사이(컴포넌트 미등록 시점)에 `NiagaraComponent->Activate/Deactivate` 를 부르는 경로(`WxRewardLibrary.cpp:81-82`, `WxItemPickup.cpp:139-147`)는 Niagara 의 미등록 활성 처리(`bAutoActivate` 재활성 여부)까지 확인하지 못해 판정을 보류했다.
  - 아이템 정의·보상 DataTable 등 데이터 에셋의 실제 값(`MaxStack`, `ItemActorClass` 설정 여부, Usable Effect 의 지속 정책 등)과 BP/WBP 내부 구조는 열어보지 않았다. 발견 4·6 의 발현 여부는 그 데이터에 달려 있다.
  - 모듈에 자동화 테스트가 없다(WxWorld 의 `WxDeviceTests.cpp` 같은 대응물 부재). 수량·슬롯 경계 로직이 순수 함수에 가까워 테스트 가치가 높은 편이라는 점만 적어 둔다. 이번 리뷰에서 빌드·에디터 실행은 하지 않았다.

---
*문서 기준 커밋 `1d91a915` · 리뷰일 2026-09-10 · 소스 20파일 — `/module-review`로 갱신*
