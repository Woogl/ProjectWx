# WxInventory — 코드 리뷰

> FastArray 기반 서버 권위 인벤토리로, 모듈 경계·코딩 규칙 준수도가 높고 리플리케이션 콜백의 델타 계산도 의도가 명확하게 주석화된 깨끗한 모듈이다. 심각(🔴) 등급 결함은 발견되지 않았으며, 이번 리뷰는 `Build.cs`/`uplugin`과 공개 헤더 전부, 그리고 인벤토리 매니저·장비 컴포넌트·픽업·보상 라이브러리·StateTree 태스크의 cpp 로직을 읽고 외부 호출부(`Source/WxGame`)까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 StateTree 보상·리필 태스크가 대상 플레이어를 0번으로 하드코딩한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:41`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:37`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 `UGameplayStatics::GetPlayerController(Owner, 0)` 로 대상을 잡는다. 엔진 헤더가 명시하듯 이 인덱스는 "로컬 플레이어 → 사용 가능한 원격 플레이어" 순의 순회 결과라 서버/클라마다 다르고 참가·이탈 시 바뀐다. 즉 데디케이티드 서버에서 B 플레이어가 상자를 열거나 체크포인트를 밟아도 보상·에스트병 리필은 먼저 접속한 A 플레이어에게 간다. 모듈 나머지(FastArray 델타, `bReplicateUsingRegisteredSubObjectList`, 서버 권위 Add/Consume)가 전부 멀티플레이 전제로 짜여 있어 이 지점만 전제가 다르다.
- **제안**: 대상 액터를 태스크 인스턴스 데이터의 바인딩 가능한 파라미터(`AActor* Target`)로 받아 상호작용을 발생시킨 실제 플레이어를 흘려주고, `UWxInventoryManagerComponent::FindInventory(Target)` 으로 해석한다. 픽업 경로(`AWxItemPickup::OnInteracted` 가 `Interactor` 를 그대로 쓴다)와 같은 모양이 된다.
- **확신도**: 중간 (현 단계가 싱글플레이 전제라면 의도된 단순화일 수 있다)

### 2. 🟡 서버 권위 가드가 `check()` 라 Shipping 에서 가드가 사라진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:268`, `:352`, `:386`, `:504`, `:569`, `:586`
- **범주**: 설계/구조
- **문제**: `AddItemDefinition`/`RemoveItemInstance`/`ConsumeItemsByDefinition`/`UseItemByDef`/`RefillItemCharges`/`EquipItemByDef` 가 모두 `check(GetOwner() && GetOwner()->HasAuthority())` 하나로 권위를 강제한다. `check` 는 `DO_CHECK=0` 인 Shipping 에서 통째로 컴파일 아웃되므로, Shipping 빌드에서는 비권위 호출이 막히지 않고 그대로 클라 로컬 인벤토리를 변조한 뒤 다음 FastArray 델타가 올 때까지 서버와 어긋난 상태로 남는다. `UWxAbility_UseItem::HandleConsumeEvent` 의 주석("UseItemByDef 는 권한을 check 한다")처럼 호출부가 이를 런타임 가드로 신뢰하고 있어 오해 소지도 있다. 같은 모듈의 `UWxEquipmentComponent::EquipItem`(`WxEquipmentComponent.cpp:37`), `AWxItemPickup::LaunchInDirection`(`WxItemPickup.cpp:61`), `UWxRewardLibrary::GrantReward`(`WxRewardLibrary.cpp:16`) 는 모두 `if (!HasAuthority()) return;` 형태의 실제 가드를 쓰고 있어 모듈 내부에서도 방식이 갈린다.
- **제안**: 프로그래머 계약 위반 탐지용 `ensure` + 조기 반환(또는 `if (!HasAuthority()) return false;`)으로 바꿔 개발 빌드에서는 시끄럽게 알리되 Shipping 에서도 실제로 막는다.
- **확신도**: 높음

### 3. 🟡 `AddItemDefinition` 이 엔트리 순회 도중 델리게이트를 브로드캐스트해 재진입에 취약하다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:278`~`:302`
- **범주**: 버그/정확성
- **문제**: 머지 루프는 `InventoryList.GetEntries()` 로 내부 `Entries` 배열 참조를 잡은 채 매 반복마다 `NotifySlotChangedFromList`/`NotifyStackChangedFromList` 로 외부 구독자를 호출한다. 구독자가 그 콜백 안에서 인벤토리를 변경하면(예: 획득 즉시 자동 사용/변환, 퀘스트 훅) 엔트리가 추가·삭제되어 `EntryIndex` 가 가리키는 슬롯이 원래와 다른 엔트리가 되고, 그다음 반복의 `AddToEntryStack(EntryIndex, ...)` 이 엉뚱한 슬롯에 수량을 얹는다. 현재 유일한 구독자인 `UWxViewModel_Inventory::HandleStackChanged` 는 읽기만 하므로 지금은 발현되지 않지만, 델리게이트가 public 이라 구독자 추가만으로 조용히 깨진다.
- **제안**: 루프 안에서는 변경만 수행해 `(Instance, NewStackCount, Delta)` 를 로컬 배열에 모으고, 순회가 끝난 뒤 한 번에 브로드캐스트한다(`ConsumeItemsByDefinition` 이 `FWxInventoryChangeResult` 로 이미 쓰는 패턴과 동일하게).
- **확신도**: 중간 (현재 구독자 구성에서는 잠재적 위험이다)

### 4. 🟡 `UseItemByDef` 가 인스턴스를 소멸시킨 뒤 그 인스턴스를 SourceObject 로 가진 GE 를 적용한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:532`, `:549`, `:556`
- **범주**: 버그/정확성
- **문제**: `Context.AddSourceObject(SourceInstance)` 로 Spec 을 만든 뒤, 비충전형 경로에서 `ConsumeItemsByDefinition(ItemDef, 1)` 이 마지막 스택을 깎으면 `FWxInventoryList::ConsumeByDefinition` 이 엔트리를 제거해 `SourceInstance` 를 참조하던 유일한 UPROPERTY 가 사라진다. 그 상태로 `ApplyGameplayEffectSpecToSelf` 를 호출하는데, `FGameplayEffectContext::SourceObject` 는 `TWeakObjectPtr` 이므로(GameplayEffectTypes.h) Instant GE 가 아닌 Duration/Infinite GE 라면 다음 GC 이후 SourceObject 가 null 이 된다. 헤더가 "SourceObject 는 인스턴스 단위 데이터 추적이 가능하도록 ItemInstance 를 사용한다"(`WxInventoryManagerComponent.cpp:530` 주석)고 밝힌 목적 자체가 무효화되며, MMC/Execution/GameplayCue 가 SourceObject 로 아이템 정보를 되읽으면 조용히 실패한다.
- **제안**: 마지막 스택 소모로 인스턴스가 사라질 수 있는 경우엔 SourceObject 를 `UWxItemDefinition`(에셋이라 수명 안정)으로 두거나, 인스턴스를 SourceObject 로 유지해야 한다면 GE 적용을 차감보다 먼저 수행한다. 최소한 "이 경로의 GE 는 Instant 여야 한다"는 제약을 Usable Fragment 주석에 명시한다.
- **확신도**: 중간 (현재 유일한 소비 아이템이 충전형 에스트병이라 비충전형 경로가 아직 데이터로 존재하지 않을 수 있다)

### 5. 🟡 비-Stackable 아이템을 수량으로 지급하면 엔트리 폭증 + O(N²) 통지가 된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:305`~`:323`, `:619`
- **범주**: 성능/안전
- **문제**: Stackable Fragment 가 없으면 `MaxStack = 1` 이라 `while (Remaining > 0)` 루프가 수량만큼 `AddEntry` 를 돌며 각각 `UWxItemInstance` 를 `NewObject` 하고 복제 서브오브젝트로 등록한다. 게다가 매 청크마다 부르는 `NotifyStackChangedFromList` 가 `GetTotalItemCountByDefinition` 으로 전체 엔트리를 다시 훑으므로 총 비용이 O(N²) 이다. 입력은 데이터에서 오며(`FWxItemRewardEntry::Quantity` 는 `ClampMin=1` 만 있고 상한이 없다, `WxRewardTableRow.h:24`), Stackable 부착을 잊은 재화·소재 아이템에 Quantity 500 을 넣으면 즉시 500개 인스턴스 + 500회 전량 스캔 + 500개 복제 서브오브젝트가 만들어진다.
- **제안**: `AddItemDefinition` 에서 신규 엔트리 생성 루프를 돌기 전에 합계 델타를 모아 통지를 1회로 줄이고(내부 `GetTotalItemCountByDefinition` 재계산도 1회), Stackable 부재 + `StackCount > 1` 조합은 경고 로그를 남겨 데이터 실수를 드러낸다.
- **확신도**: 중간 (정상 데이터라면 발현되지 않는 데이터 사고 대비 성격이다)

### 6. 🟢 배치 복제 콜백에서 `NewCount` 와 `Delta` 가 서로 다른 시점을 가리킨다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:72`~`:113`, `:619`
- **범주**: 버그/정확성
- **문제**: 한 번의 델타 직렬화에서 여러 엔트리가 추가/제거될 때 `PostReplicatedAdd`/`PostReplicatedChange` 는 인덱스마다 통지를 발행하는데, `NotifyStackChangedFromList` 가 계산하는 `NewCount` 는 배열에 이미 전부 반영된 최종 총량인 반면 `Delta` 는 해당 엔트리 몫뿐이다. 결과적으로 첫 통지부터 `NewCount` 가 최종값으로 점프하고, `Delta` 를 누적하는 소비자와 `NewCount` 를 그대로 쓰는 소비자가 배치 중간에 서로 다른 값을 본다. `PreReplicatedRemove` 는 자기 엔트리만 `StackCount = 0` 으로 내려(`:64`) 보정하므로 같은 배치의 다른 제거 대상은 여전히 총량에 남아 동일한 어긋남이 생긴다. 최종 수렴값은 항상 정확하고 현 구독자(`UWxViewModel_Inventory`)는 `NewCount` 만 쓰므로 실제 증상은 없다.
- **제안**: 통지를 배치 단위로 합산해 `(ItemDef, 최종 NewCount, 합산 Delta)` 를 한 번만 발행하거나, `Delta` 는 참고용이며 배치 중간값이 나올 수 있음을 델리게이트 주석에 명시한다.
- **확신도**: 낮음 (의도된 설계일 수 있음 — 소비자가 `NewCount` 만 쓰는 전제라면 무해하다)

### 7. 🟢 장비 경로 전체가 호출부 0건인 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h:179`, `:234`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h:28`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체 검색 결과 `UWxInventoryManagerComponent::EquipItemByDef` 와 `RemoveItemInstance` 의 호출부가 0건이며, `UWxEquipmentComponent::EquipItem` 의 유일한 호출부가 `EquipItemByDef` 라 `EquippedItemDef` 는 항상 null 이다(헤더 주석도 그렇게 밝히고 있다). 즉 `UWxEquipmentComponent` 의 GE 라이프사이클·`OnEquipVisualChanged` 방송 전체가 미검증 상태로 남아 있고, `AWxCharacterBase` 가 `WxCharacterBase.cpp:84` 에서 구독만 걸어둔 채 아무것도 받지 못한다. 헤더 주석이 지적한 "늦게 relevant 해진 클라가 초기 `OnRep_EquippedItemDef` 를 구독보다 먼저 받아 방송을 유실하는데 되물을 pull API 가 없다"는 문제도 경로를 여는 순간 바로 터진다.
- **제안**: 장비 기능을 당분간 안 쓸 거면 경로를 제거해 유지보수 표면을 줄이고, 열 계획이면 `EquipItemByDef` 호출부(UI 또는 어빌리티)와 함께 현재 장착 상태를 되묻는 getter(`GetEquippedItemDef`)를 같이 넣어 초기 복제 유실을 닫는다.
- **확신도**: 높음 (기능 미완이라는 사실 자체는 헤더에 명시돼 있어 인지된 상태다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, 교차 확인용 `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`
- **규칙 준수 확인 결과**: 위반 없음. 22개 소스 전부 첫 줄 저작권 표기가 있고, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward`(Blueprint Function Library) 한 곳뿐이며, 람다·`FORCEINLINE`·인라인 정의가 없고 헤더 정의 2건(`FindFragmentByClass<T>`, StateTree `GetInstanceDataType()`)은 모두 규칙 6 예외 사유가 주석으로 달려 있다. `Build.cs`/`uplugin` 의존이 `WxCore` 로 한정돼 플러그인 경계도 지켜진다. 델리게이트 콜백은 이 모듈 안에 바인딩이 없어 `Handle` 접두사 규칙의 판정 대상이 없다(`OnRep_*` 는 엔진 RepNotify 규약이라 대상 아님).
- **미검토 / 한계**: 아이템/보상 데이터 에셋(`UWxItemDefinition`, `FWxRewardTableRow` DataTable)의 실제 내용은 열지 않아, 발견 4·5 가 현재 데이터에서 실제로 발현되는지는 확인하지 못했다. 리플리케이션 관련 지적(발견 6)은 코드 정적 분석 결과이며 실제 네트워크 세션에서 재현 검증하지 않았다. BP/WBP 내부 구조는 범위 밖이라 `AWxItemPickup` 파생 BP 의 설정(콜리전·Niagara 오토액티베이트 등)은 보지 않았다.

---
*문서 기준 커밋 `49cc6a81` · 리뷰일 2026-08-27 · 소스 22파일 — `/module-review`로 갱신*
