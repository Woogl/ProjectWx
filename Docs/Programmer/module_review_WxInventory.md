# WxInventory — 코드 리뷰

> 도메인 경계·복제 모델·통지 대칭성이 잘 정돈된 모듈이다. 서버 변경 경로와 클라 FastArray 콜백이 같은 `Notify*` 진입점으로 수렴하는 구조가 실제로 델타 합이 맞고, 프로젝트 코딩 규칙 위반은 한 건도 없다. 이번 리뷰는 22개 소스 전부를 훑고 인벤토리 매니저·픽업·보상 라이브러리·장비 컴포넌트의 cpp 로직까지 내려가 확인했다(BP/WBP 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 MaxStack 이 0 이하면 AddItemDefinition 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:271`, `:305-323`
- **범주**: 성능/안전
- **문제**: `MaxStack = Stackable ? Stackable->MaxStack : 1` 로 받은 값이 0 이하면 `ChunkCount = FMath::Min(MaxStack, Remaining)` 이 0 이 되어 `Remaining` 이 줄지 않는다. `while (Remaining > 0)` 이 매 반복마다 `AddEntry` 로 `UWxItemInstance` 를 새로 할당하며 영원히 돈다 — 게임 스레드 행 + OOM. 유일한 방어선인 `UWxItemFragment_Stackable::MaxStack` 의 `meta = (ClampMin = "1")` 은 디테일 패널 입력만 막을 뿐, C++ 서브클래스 기본값·에셋 임포트·Python/자동화로 들어온 값은 걸러내지 못한다.
- **제안**: `const int32 MaxStack = FMath::Max(1, Stackable ? Stackable->MaxStack : 1);` 로 진입 시점에 하한을 강제한다.
- **확신도**: 중간 (경로는 확실히 도달 가능하나 현실적 발생 확률은 낮음)

### 2. 🟡 보상 직접 지급·충전 리필 대상이 0번 플레이어로 하드코딩돼 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:41`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:37`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 `UGameplayStatics::GetPlayerController(Owner, 0)` 으로 대상을 잡는다. 모듈 전체가 FastArray + 서버 권위로 멀티플레이를 전제해 설계돼 있는데, 이 두 진입점만 "0번 컨트롤러" 라는 싱글플레이 가정을 박아 놨다. 데디케이티드 서버에서 인덱스 0 은 접속 순서에 따라 결정되므로, 재화(Pickup Fragment 없는 보상)는 기믹을 연 플레이어가 아닌 엉뚱한 플레이어에게 들어가고 체크포인트 리필도 한 명만 받는다. 픽업 스폰 경로는 월드에 액터를 놓으므로 영향이 없어, 증상이 "재화만 안 들어온다"로 나타나 원인 추적이 어렵다.
- **제안**: 기믹을 발동한 주체(상호작용 instigator)를 인스턴스 데이터/바인딩으로 받아 그 액터의 인벤토리를 대상으로 삼는다. 싱글플레이 고정이 의도라면 태스크 주석이 아니라 모듈 README 의 경계 절에 그 전제를 명시한다.
- **확신도**: 중간 (싱글플레이 우선 개발 단계의 의도된 단순화일 수 있음)

### 3. 🟡 장비 경로 전체가 도달 불가능한 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:584-610`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:34`, `:82`, `:119`
- **범주**: 중복/복잡도
- **문제**: `EquipItemByDef` 의 호출부가 저장소 전체에서 0건이고 `BlueprintCallable` 도 아니라 BP 진입도 불가하다. `UWxEquipmentComponent::EquipItem` 의 유일한 호출부가 `EquipItemByDef` 이므로 `EquippedItemDef` 는 항상 null 이고, 복제·RepNotify·`OnEquipVisualChanged` 방송·EquipEffects GE 라이프사이클(약 120줄)이 한 번도 실행된 적이 없다. `AWxCharacterBase` 가 `OnEquipVisualChanged` 를 구독(`Source/WxGame/Character/WxCharacterBase.cpp:83`)하고 있어 게임 모듈 쪽 코드까지 함께 미검증 상태다. `RemoveItemInstance`(`:345-377`)도 호출부 0건이다. 헤더 주석이 이 사실을 정직하게 적어 뒀지만, 검증된 적 없는 복제 코드가 계속 쌓이면 나중에 켤 때 한꺼번에 터진다.
- **제안**: 트리거(장비 UI 또는 어빌리티)를 붙여 경로를 닫거나, 로드맵이 멀면 `UWxEquipmentComponent` 와 `EquipItemByDef`/`RemoveItemInstance` 를 삭제하고 필요 시점에 되살린다. 켤 때는 헤더가 지적한 "늦게 relevant 해진 클라가 초기 RepNotify 를 구독보다 먼저 받아 방송을 유실하는" 문제도 같이 처리해야 한다(pull API 추가).
- **확신도**: 높음

### 4. 🟡 UseItemByDef 가 SourceObject 로 실은 인스턴스를 제거한 뒤에 GE 를 적용한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:541-557`
- **범주**: 버그/정확성
- **문제**: `FGameplayEffectContext` 에 `SourceInstance` 를 `AddSourceObject` 로 실어 Spec 을 만든 뒤(`:532`), 비충전형 분기에서 `ConsumeItemsByDefinition(ItemDef, 1)` 로 마지막 스택을 차감한다. 스택이 0 이 되면 엔트리가 제거되고 `UnregisterReplicatedInstance` 까지 돌아 그 인스턴스는 복제 대상에서 빠지고 참조도 끊긴다. 그 상태에서 `:556` 이 GE 를 적용하므로, 이펙트가 `SourceObject` 로 인스턴스 데이터(예: 충전량, Fragment)를 읽으려 하면 서버에선 GC 대상 객체를, 클라에선 해석 불가로 null 을 보게 된다. 지금은 소비 아이템이 충전형 에스트병 하나뿐이라 이 분기를 타지 않지만, `RequestUseConsumable` 주석이 명시하듯 소비 아이템 종류 확장이 예정돼 있다.
- **제안**: 비충전형은 GE 를 먼저 적용하고 차감을 뒤로 돌리거나, 인스턴스가 사라질 수 있는 경우엔 `SourceObject` 를 `UWxItemDefinition`(자산이라 수명이 안정적)으로 싣는다.
- **확신도**: 중간

### 5. 🟡 공개 API 의 권위 가드를 check() 로 잡고 있어 Shipping 에서는 가드가 사라진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:268`, `:352`, `:386`, `:504`, `:569`, `:586`
- **범주**: 설계/구조
- **문제**: `check(GetOwner() && GetOwner()->HasAuthority())` 는 `DO_CHECK` 가 꺼진 Shipping 에서 통째로 컴파일 아웃된다. 즉 같은 오호출이 Development 에선 즉시 크래시, Shipping 에선 조용히 통과해 클라 로컬에서 `InventoryList` 를 변조한다(다음 서버 델타가 덮어쓰며 UI 플리커/디싱크로 나타난다). 게다가 `AWxItemPickup::OnInteracted` → `AddItemDefinition` 경로의 "서버에서만 호출된다"는 보장은 WxWorld 쪽 주석 규약일 뿐 코드로 강제돼 있지 않아, 상호작용 경로가 예측을 도입하는 순간 개발 빌드가 크래시한다.
- **제안**: 구성과 무관하게 같은 동작을 하도록 `if (!GetOwner() || !GetOwner()->HasAuthority()) { return ...; }` 형태의 조기 반환(필요하면 `ensure` 병행)으로 바꾼다.
- **확신도**: 중간 (현재 모든 호출부가 이미 권위 게이팅돼 있어 실제 발화는 없음)

### 6. 🟡 드랍·복제 수신 경로에서 동기 에셋 로드가 게임 스레드를 막는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp:42`, `:62`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:135`, `:140`
- **범주**: 성능/안전
- **문제**: 서버는 적 처치마다 `GrantReward` 에서 `Reward.Item.LoadSynchronous()` 와 `PickupFragment->ItemActorClass.LoadSynchronous()` 를, 클라는 `OnRep_ItemDef` → `ApplyPickupVisual` 에서 `Mesh.LoadSynchronous()` 와 `NiagaraSystem.LoadSynchronous()` 를 돌린다. 즉 새 아이템 종류가 처음 드랍되는 프레임에 서버·클라 모두 블로킹 로드가 걸린다. 코드 주석은 "시작 시 1회 같은 산발적 호출"을 근거로 들지만(`WxInventoryManagerComponent.cpp:337`), 적 드랍은 전투 중 반복 발생하는 이벤트라 그 전제가 성립하지 않는다. 캐시가 도는 두 번째부터는 무해하므로 증상은 "특정 적을 처음 잡을 때만 히치"로 나타난다.
- **제안**: 최소한 픽업 비주얼(메시/나이아가라)은 `FStreamableManager` 비동기 요청 + 도착 시 반영으로 바꾸고, 자주 나오는 드랍 테이블의 `UWxItemDefinition`·`ItemActorClass` 는 레벨/Experience 로드 시점에 미리 예열한다.
- **확신도**: 중간 (의도적 정책으로 명시돼 있으나 근거가 드랍 빈도와 맞지 않음)

### 7. 🟢 GetPrimaryAssetId 가 AssetManager 에 등록되지 않은 "WxItem" 타입을 반환한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp:12-15`
- **범주**: 버그/정확성
- **문제**: `FPrimaryAssetId(TEXT("WxItem"), GetFName())` 를 돌려주지만 `Config/DefaultGame.ini:44-50` 의 `PrimaryAssetTypesToScan` 에 `WxItem` 항목이 없다(등록된 것은 Map / PrimaryAssetLabel / GameFeatureData / WxExperienceDefinition / WxExperienceActionSet 뿐). 타입이 스캔 대상이 아니므로 아이템 정의는 AssetManager 에 프라이머리 애셋으로 등록되지 않고, ID 기반 비동기 로드·쿡 룰(`AlwaysCook`)·청킹이 전부 무효다. 즉 이 오버라이드는 기본 구현 대비 얻는 것이 없으면서 "AssetManager 로 관리된다"는 잘못된 인상만 준다. 아이템 정의(`Content/Item/DA_*.uasset`)는 `TSoftObjectPtr` 참조로만 도달 가능하고 `/Game/Item` 은 `DirectoriesToAlwaysCook` 에도 없으므로, 쿠커의 소프트 참조 추적에만 의존하는 상태다.
- **제안**: `PrimaryAssetTypesToScan` 에 `WxItem`(AssetBaseClass=`/Script/WxInventory.WxItemDefinition`, Directories=`/Game/Item`, CookRule=AlwaysCook)을 추가하거나, 관리할 의도가 없다면 오버라이드를 지우고 기본 구현을 쓴다.
- **확신도**: 중간

### 8. 🟢 픽업 헤더 주석이 폐기된 메시 단위 상호작용 계약을 설명하고 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h:17`
- **범주**: 중복/복잡도
- **문제**: "메시 자체가 상호작용 영역이며 … 계약 인터페이스(WxCore)로 자기 메시를 답하므로" 라고 적혀 있으나, 커밋 `ce04ce1f` 이후 `IWxInteractable`(`Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:13`)은 "상호작용은 액터 단위다 — 액터 안의 특정 메시만 상호작용 영역이 되는 개념은 없다"로 바뀌었고, 메시를 답하는 API 자체가 없다. 코드는 정상이지만(메시에 쿼리 콜리전이 켜져 있어 `IsActorInRange` 전제를 만족한다) 주석만 이전 계약에 남아 있어, 이 파일을 처음 읽는 사람이 존재하지 않는 API 를 찾게 된다.
- **제안**: "액터 단위 상호작용 대상이며, 스캔·사거리 전제를 만족하도록 메시에 쿼리 콜리전을 켜 둔다" 취지로 문구를 갱신한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`, 교차 확인용으로 `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Config/DefaultGame.ini`
- **미검토 / 한계**:
  - 규칙 준수는 기계적으로 전수 확인했고 위반 0건이다 — `WxCore` 외 Wx 플러그인 의존 없음, 전 파일 첫 줄 Copyright 일치, 람다·`FORCEINLINE` 0건, 델리게이트 바인딩 자체가 없어 `Handle` prefix 대상 없음, `BlueprintCallable` 은 `UWxRewardLibrary::GrantReward`(BP Function Library) 단 1건으로 규칙 범위 안. `GetInstanceDataType()`·`FindFragmentByClass<T>` 의 헤더 내 정의는 코드 주석에 예외 근거가 명시돼 있어 위반으로 세지 않았다.
  - `FWxInventoryList` 의 서버/클라 통지 델타 합이 Add·Merge·Consume·Remove 네 경로에서 일치함을 코드 추적으로 확인했으나, 실제 네트워크 세션(특히 한 프레임에 다중 슬롯이 변하는 경우)의 중간 총량 브로드캐스트가 UI 에 어떻게 보이는지는 실행 검증하지 않았다.
  - `FWxInventoryList::OwnerComponent` 를 생성자에서 `this` 로 잡는 Lyra 패턴은 네이티브 클래스에서만 안전하다(BP 서브클래스를 만들면 CDO 값이 덮어쓴다). 현재 BP 서브클래스가 없어 발견으로 올리지 않았다.
  - BP/WBP 자산 내부 구조와 `Content/Item/DA_*` 데이터값(MaxStack 등 실제 설정값)은 범위 밖이다.

---
*문서 기준 커밋 `ce04ce1f` · 리뷰일 2026-08-21 · 소스 22파일 — `/module-review`로 갱신*
