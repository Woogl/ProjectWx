# WxInventory — 코드 리뷰

> 도메인 경계(WxCore 만 참조)와 서버/클라 통지 수렴 설계가 잘 잡혀 있고, 코딩 규칙 위반은 발견되지 않은 건강한 모듈이다. 22개 소스 전부를 읽었고, `WxInventoryManagerComponent`(리스트 복제·Add/Consume/Use)·`WxEquipmentComponent`·`WxItemPickup`·`WxRewardLibrary`·StateTree 태스크는 cpp 까지 깊게 봤으며, `Source/WxGame` 의 호출부(UseItem 어빌리티·GameMode·ViewModel)와 UE 5.8 엔진 소스(FastArray 언맵 처리, 서브오브젝트 복제 순서, 컴포넌트 오토액티베이트)로 가정을 교차 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `MaxStack <= 0` 이면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:271`, `:305-323`
- **범주**: 버그/정확성
- **문제**: `MaxStack` 은 `Stackable` Fragment 값을 그대로 쓰는데(`:271`), 이 값이 0 이하이면 `MaxStack > 1` 병합 분기를 건너뛴 뒤 `while (Remaining > 0)` 루프에서 `ChunkCount = FMath::Min(MaxStack, Remaining) <= 0` 이 되어 `Remaining` 이 줄지 않는다. 매 반복마다 `AddEntry` 가 새 `UWxItemInstance` 를 `NewObject` 로 만들고 엔트리를 추가하므로 서버가 행 + 메모리 폭주로 죽는다. 방어는 `UWxItemFragment_Stackable::MaxStack` 의 `ClampMin = "1"`(`Public/Items/WxItemFragment.h:122`) 하나뿐인데, ClampMin 은 디테일 패널 입력에만 적용되고 이미 직렬화된 값·스크립트/툴 경로의 대입은 막지 못한다. `FWxInventoryList::AddEntry` 에도 `StackCount` 검증이 없다.
- **제안**: `const int32 MaxStack = FMath::Max(1, Stackable ? Stackable->MaxStack : 1);` 로 소스에서 하한을 강제한다(또는 `AddEntry` 진입 시 `check(StackCount > 0)`).
- **확신도**: 높음 (코드 경로는 확실, 발동 전제는 데이터 오류)

### 2. 🟡 보상/리필 StateTree 태스크가 지급 대상을 0번 플레이어로 하드코딩한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:41`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:37`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 서버 권위에서 `UGameplayStatics::GetPlayerController(Owner, 0)` 로 대상을 고른다. 데디케이티드 서버에서 인덱스 0 은 "이 태스크를 유발한 플레이어"가 아니라 단지 컨트롤러 목록의 첫 항목이다. 결과적으로 멀티플레이에서는 플레이어 B 가 연 상자의 재화(Pickup Fragment 없는 보상)가 플레이어 A 에게 들어가고, 화톳불 리필도 A 의 병만 채워진다. 모듈 전반이 서버 권위 복제로 정성껏 짜여 있는 것에 비해 이 지점만 싱글플레이 전제가 남아 있다.
- **제안**: 상호작용 주체를 태스크 인스턴스 데이터의 바인딩 가능한 파라미터(`AActor* Target`)로 노출해 기믹 StateTree 가 실제 Interactor 를 넘기게 한다. 단기적으로는 헤더 주석에 "싱글플레이 전제"임을 못 박고 MP 전환 시 손볼 지점으로 표시한다.
- **확신도**: 중간 (현 시점 싱글플레이 우선 설계라면 의도된 단순화일 수 있음)

### 3. 🟡 서버 권한 계약이 `check()` 로만 강제되어 Shipping 에서 사라진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:268`, `:352`, `:386`, `:504`, `:569`, `:586`
- **범주**: 설계/구조
- **문제**: 상태 변경 API 6종이 전부 `check(GetOwner() && GetOwner()->HasAuthority())` 로 권한을 강제한다. `check` 는 Shipping 에서 컴파일 아웃되므로, 잘못된 클라 호출이 개발 빌드에서는 하드 크래시(에디터 종료)로, 출시 빌드에서는 **무음 로컬 변경 → 서버와의 desync** 로 갈라진다. 두 결과 모두 바람직하지 않고, 실패 모드가 빌드 구성에 따라 달라진다. 같은 계약이 `AWxItemPickup::SetItemDef`/`OnInteracted`(`Private/Items/WxItemPickup.cpp:52`, `:77`)에서는 주석으로만 존재해 강제 수단조차 없다.
- **제안**: `if (!GetOwner() || !GetOwner()->HasAuthority()) { ensureMsgf(false, ...); return false; }` 형태로 통일해 어떤 빌드에서도 "거부 + 진단 로그"로 수렴시킨다. `UWxEquipmentComponent::EquipItem`(`Private/Inventory/WxEquipmentComponent.cpp:37`)이 이미 이 모양(조기 반환)이라 모듈 내 선례도 있다.
- **확신도**: 중간

### 4. 🟡 `AddItemDefinition` 병합 루프가 인덱스 순회 도중 델리게이트를 방송한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:278-302`
- **범주**: 버그/정확성
- **문제**: `const TArray<FWxInventoryEntry>& Entries` 를 `EntryIndex` 로 순회하는 도중 `NotifySlotChangedFromList`/`NotifyStackChangedFromList` 를 방송한다(`:295-296`). 구독자가 방송 안에서 인벤토리를 변경하면(자동 소비·자동 정리 등) 엔트리가 제거·재정렬되어 이후 반복의 `EntryIndex` 가 다른 슬롯을 가리키고, `AddToEntryStack` 은 `Entries[EntryIndex]`(`:160`)를 범위 검사 없이 인덱싱한다. 같은 모듈의 `ConsumeByDefinition` 은 이미 "변경을 모아 반환하고 호출자가 나중에 통지"(`:166-198`, `:393-403`) 패턴을 쓰고 있어 규약이 한 파일 안에서 엇갈린다.
- **제안**: 병합·신규 생성 결과를 `TArray<FWxInventoryChangeResult>` 로 모은 뒤 루프를 빠져나와 방송한다 — Consume 경로와 같은 모양으로 맞춘다.
- **확신도**: 중간 (현재 구독자는 재진입하지 않으므로 잠재 위험)

### 5. 🟡 장비 경로 전체가 배선만 있고 트리거가 없는 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h:180`, `:236`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h:28-30`
- **범주**: 중복/복잡도
- **문제**: `EquipItemByDef` → `UWxEquipmentComponent::EquipItem` → `OnEquipVisualChanged`/`EquipEffects` 로 이어지는 사슬의 최초 호출부가 저장소 전체에 0건이다(`BlueprintCallable` 도 아니라 BP 진입도 없다). `RemoveItemInstance` 도 마찬가지다. 즉 `UWxEquipmentComponent`(캐릭터가 `WxCharacterBase.cpp:38` 에서 항상 생성) · `UWxItemFragment_Equippable` · 관련 복제 프로퍼티가 런타임에 한 번도 동작하지 않으면서 유지보수·리뷰 비용만 발생시킨다. 헤더 주석이 이 사실과 함께 "늦게 relevant 해진 클라는 초기 RepNotify 가 구독보다 앞서면 방송을 유실하는데 되물을 pull API 가 없다"는 미해결 결함까지 이미 기록해 두었다.
- **제안**: 경로를 닫을 계획이 가까우면 진입점(UI/입력)을 붙이면서 pull API(`GetEquippedItemDef()` 또는 구독 즉시 현재 상태 1회 방송)를 같이 넣고, 계획이 멀면 사슬을 통째로 제거해 재도입 시 새로 짠다.
- **확신도**: 높음 (사실 확인 완료 — 호출부 전수 검색으로 0건)

### 6. 🟢 `UWxRewardLibrary::GrantReward` 가 `World` 를 검증 없이 역참조한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:38`, `:70`
- **범주**: 성능/안전
- **문제**: `SourceActor->GetWorld()` 결과를 널 검사 없이 `World->SpawnActorDeferred` 에 쓴다. 주 호출부가 적 사망 처리(`Source/WxGame/Character/WxEnemyCharacter.cpp:93`)라 액터가 이미 월드에서 떨어져 나간 시점에 불릴 여지가 있고, 다른 두 널 가드(`SourceActor`, `Row`)와 방어 수준이 어긋난다.
- **제안**: `World` 를 얻는 자리에서 널이면 조기 반환한다(한 줄).
- **확신도**: 높음

### 7. 🟢 `UWxItemFragment_Charges` 의 에디터 표시 이름이 "Refill" 이라 클래스 의미와 어긋난다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h:87`
- **범주**: 중복/복잡도
- **문제**: 나머지 Fragment 는 모두 `DisplayName` 이 클래스 접미사와 일치하는데(Equippable/Usable/Stackable/Pickup/Grade), 이 클래스만 `DisplayName = "Refill"` 이다. 기획자는 디테일 패널에서 "Refill" 을 고르고 문서·코드에서는 Charges 를 읽게 되어 용어가 갈린다. Refill 은 이 Fragment 가 아니라 `RefillItemCharges` 동작의 이름이다.
- **제안**: `DisplayName = "Charges"` 로 정정한다.
- **확신도**: 높음

### 8. 🟢 정의 단위 합계를 통지 때마다 전수 재계산한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:612-621`
- **범주**: 성능/안전
- **문제**: `NotifyStackChangedFromList` 는 매 호출마다 `GetTotalItemCountByDefinition` 으로 엔트리 전체를 훑는다. 그런데 이 함수는 슬롯 단위로 호출된다 — `AddItemDefinition` 의 병합/신규 청크마다(`:296`, `:312`), `PostReplicatedAdd`/`PostReplicatedChange` 의 인덱스마다(`:88`, `:110`). 초기 인벤토리 복제처럼 N개 엔트리가 한꺼번에 도착하면 O(N²)가 된다. 인벤토리 크기가 작아 현재는 무해하지만, 슬롯 수가 커지면 접속 프레임에 눈에 띈다.
- **제안**: 실제로 걸릴 때 손보면 되는 수준이다. 필요해지면 정의별 합계를 캐시하거나, 한 배치 안의 통지를 정의 단위로 합산해 한 번만 발행한다.
- **확신도**: 높음 (동작은 확실, 현시점 영향은 미미)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, 나머지 Public 헤더 전부
- **확인했지만 문제가 아니었던 것**(재조사 방지용):
  - `FWxInventoryList InventoryList(this)` 의 `OwnerComponent` 는 네이티브 클래스라 `FObjectInitializer::InitProperties` 가 CDO 값으로 덮어쓰지 않는다.
  - `bReplicateUsingRegisteredSubObjectList` 경로에서 엔진은 컴포넌트 서브오브젝트를 컴포넌트 프로퍼티보다 **먼저** 기록한다(`DataChannel.cpp` `ReplicateRegisteredSubObjects`). 따라서 `PostReplicatedAdd` 시점에 `Entry.Instance` 는 이미 해석되어 있고, 델타 기반 통지가 유실되지 않는다.
  - `AWxItemPickup::ApplyPickupVisual` 이 Deferred 스폰 구간(컴포넌트 미등록)에서 `NiagaraComponent->Activate` 를 부르지만 조기 반환된다. 다만 `bAutoActivate` + `AActor::InitializeComponents` 가 등록 후 다시 활성화하므로 이펙트는 정상 재생된다.
  - 서버 변경 경로와 클라 복제 콜백 경로는 리슨 서버 포함 어느 구성에서도 이중 방송되지 않는다.
  - 코딩 규칙(Copyright 첫 줄, `Wx` prefix, 람다 부재, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward` 하나뿐, override 의 `Super::` 호출, `WxCore` 외 Wx 플러그인 미참조) 위반 없음. 헤더 내 함수 정의는 템플릿과 StateTree `GetInstanceDataType()` 뿐이며 둘 다 규칙 6 예외 사유가 주석에 명시돼 있다.
- **미검토 / 한계**: `Content/` 아래 `UWxItemDefinition`·`FWxRewardTableRow` 데이터 자산의 실제 값(예: 실제로 `MaxStack <= 0` 인 자산이 있는지)은 확인하지 않았다. BP/WBP 내부와 StateTree 자산의 바인딩 구성도 범위 밖이다.

---
*문서 기준 커밋 `6b77c352` · 리뷰일 2026-08-21 · 소스 22파일 — `/module-review`로 갱신*
