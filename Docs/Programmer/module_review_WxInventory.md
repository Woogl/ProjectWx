# WxInventory — 코드 리뷰

> 모듈 경계와 코딩 규칙은 흠이 없고(Wx 의존은 `WxCore` 하나, 저작권 첫 줄 24/24, 람다·`FORCEINLINE` 0건, 헤더 본문 정의 4건은 모두 예외 사유 주석 보유, `BlueprintCallable` 은 BP Function Library 의 `UWxRewardLibrary::GrantReward` 하나), 서버 권위·FastArray 복제 골격도 건전하다 — 실제로 드러나는 문제는 통지 입자도가 경로마다 달라 획득 연출 수량이 쪼개지는 1건이고 나머지는 확장점·저작 데이터·실패 경로에 걸린 잠재 결함이다. 이번 리뷰는 소스 24파일을 모두 읽고 인벤토리 컴포넌트·아이템 인스턴스·사용 컴포넌트를 정독했으며, 복제 순서와 FastArray 콜백 재호출은 UE 5.8 엔진 소스로, 소비자 계약은 `Source/WxGame` 의 뷰모델·어빌리티로 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 7 |

## 결과

### 1. 🟡 스택 변경 통지의 입자도가 경로마다 달라, 한 번의 획득이 여러 토스트로 쪼개진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:302-303`, `:318-319`, `:412`, `:66-67`, `:86-87`, `:108-109`, `:597` / 소비자 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104-122`
- **범주**: 버그/정확성
- **문제**: 같은 `OnInventoryStackChanged` 가 경로마다 다른 단위로 발행된다. 서버 추가 경로는 머지·분할된 슬롯마다 1회(302-303·318-319행), 서버 소비 경로는 배치 전체에 1회(412행), 클라이언트 복제 경로는 엔트리마다 1회(66-67·86-87·108-109행)다. 구독자 `UWxViewModel_Inventory::HandleStackChanged` 는 `Delta > 0` 이 올 때마다 획득 연출용 VM 을 새로 만들어 `AcquiredCount = Delta` 로 `LastAcquiredItem` 에 넣는다(`WxViewModel_Inventory.cpp:110-118`, 헤더 `WxViewModel_Inventory.h:73-74` 는 발행마다 토스트가 독립적으로 뜨는 것을 의도로 적고 있다). 그래서 Stackable 없는 아이템 5개를 보상으로 받으면 "x1" 이 5번, 97/99 슬롯에 5개를 더하면 "x2" 와 "x3" 이 따로 뜬다. 스탠드얼론·서버에서 그대로 재현된다. 덤으로 발행마다 597행이 엔트리 전체를 다시 합산하고 구독자가 O(N²) 인 `RefreshAllItems()`(`WxViewModel_Inventory.cpp:121`, `:124-171`)를 돌려, 대량 지급 1회가 슬롯 수만큼 증폭된다.
- **제안**: 발행 단위를 "변경 배치당 ItemDef 1회"로 통일한다. 서버는 `AddItemDefinition` 이 델타를 누적해 함수 끝에서 한 번 발행한다(소비 경로가 이미 이 형태이고, 슬롯 단위 정보는 `OnInventorySlotChanged` 가 계속 담당한다). 클라이언트는 한 번의 수신에서 Change(+2)·Add(+3) 콜백이 따로 호출되므로, 세 콜백에서 ItemDef 별 델타를 모았다가 가장 마지막에 호출되는 `PostReplicatedReceive`(119-126행)에서 한 번 발행해야 서버와 같은 수량이 나온다.
- **확신도**: 높음

### 2. 🟢 `AddEntry` 가 가상 확장점 호출 너머로 `Entries` 원소 참조를 붙든다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:135-150`
- **범주**: 버그/정확성
- **문제**: 135행 `Entries.AddDefaulted_GetRef()` 로 얻은 원소 참조 `NewEntry` 를 들고 145행에서 `Fragment->OnInstanceCreated(NewEntry.Instance)` 를 호출한 뒤 149-150행에서 다시 쓴다. `OnInstanceCreated` 는 `Public/Items/WxItemFragment.h:39-40` 과 README 가 기능 확장점으로 광고하는 가상 함수인데, 파생 Fragment 가 그 안에서 같은 인벤토리에 아이템을 추가·차감하면(묶음 아이템 개봉 등) `Entries` 버퍼가 재할당·이동되어 `MarkItemDirty(NewEntry)` 가 해제된 메모리를 쓴다. 현재 오버라이드는 충전량만 채우는 `UWxItemFragment_Charges` 하나라 발현되지 않는다.
- **제안**: 인스턴스를 로컬 변수로 먼저 완성(`NewObject` → `SetItemDef` → Fragment 루프)한 뒤, 마지막에 `AddDefaulted_GetRef()` 로 엔트리를 만들어 대입과 `MarkItemDirty` 까지 중간 호출 없이 끝낸다.
- **확신도**: 중간

### 3. 🟢 `MaxStack` 이 0 이하면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:278`, `:312-330`
- **범주**: 성능/안전
- **문제**: 278행은 `Stackable->MaxStack` 을 검증 없이 쓴다. 0 이면 314행 `ChunkCount` 가 0 이라 325행이 진전을 만들지 못하고, 음수면 `Remaining` 이 오히려 늘어나, while 루프가 매 회전 엔트리와 `UWxItemInstance` 를 만들며 멈추지 않는다(행 + OOM). `ClampMin = "1"`(`Public/Items/WxItemFragment.h:98`)은 에디터 입력 위젯에만 적용되므로, 클램프가 붙기 전에 저장된 값이나 스크립트·툴셋이 리플렉션으로 쓴 값은 막지 못한다.
- **제안**: 278행을 `const int32 MaxStack = FMath::Max(1, Stackable ? Stackable->MaxStack : 1);` 로 한 번 방어한다.
- **확신도**: 중간

### 4. 🟢 Stackable 과 Charges 를 함께 붙이면 스택 전체가 충전량 하나를 공유한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:283-310`, `:547-554`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp:11-19`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h:30-64`
- **범주**: 설계/구조
- **문제**: 충전량은 `UWxItemInstance` 단위인데 Stackable 은 여러 개를 인스턴스 하나로 머지한다. 두 Fragment 를 함께 부착한 아이템을 3개 획득하면 첫 획득만 `OnInstanceCreated` 로 `MaxCharges` 를 채우고(`WxItemFragment.cpp:17`) 이후 획득은 머지 경로(283-310행)라 인스턴스를 만들지 않는다. `UseItemByDef` 는 Charges 가 있으면 스택을 건드리지 않고 그 하나의 충전량만 줄이므로(547-554행), 3개를 들고도 총 `MaxCharges` 회만 쓸 수 있다. `UWxItemDefinition` 에 `IsDataValid` 가 없어 저작 시 경고도 없다. 기능 축을 직교로 두는 Fragment 컴포지션은 의도이지만 이 조합만은 성립하지 않는다.
- **제안**: `UWxItemDefinition::IsDataValid` 에서 Stackable + Charges 동시 부착을 에러로 잡고, Charges 클래스 주석(`Public/Items/WxItemFragment.h:57-62`)에 Usable 과 달리 Stackable 과는 함께 쓸 수 없다고 남긴다.
- **확신도**: 중간

### 5. 🟢 `HandleUseItemEvent` 가 `UseItemByDef` 의 실패를 버려, 마시는 모션만 나가고 흔적이 남지 않는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp:83-91`
- **범주**: 버그/정확성
- **문제**: 90행은 `UseItemByDef` 의 반환값을 버리고, 88행에서 인벤토리를 못 찾아도 조용히 끝난다. `UseItemByDef` 는 6개 경로에서 false 를 돌려주는데(`WxInventoryComponent.cpp:508` 널 정의, `:516` Usable 없음, `:522` 사용 가능 인스턴스 없음, `:533` 대상 ASC 없음, `:543` GE Spec 무효, `:557` 차감 실패), 어빌리티 발동 시점의 사전 검증(`Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp:35`)과 AnimNotify 시점 사이에 상태가 바뀌면 몽타주를 끝까지 보고도 회복이 없고, 83-84행에서 `PendingItemDefinition` 은 이미 비워져 재시도 여지도 없다. 같은 모듈의 픽업·보상 실패 경로는 진단 로그를 남기는데(`WxItemPickup.cpp:93`, `WxRewardLibrary.cpp:62`·`:70`) 이 파일에는 로그 카테고리가 없어, Effect 클래스 누락 같은 저작 실수도 묻힌다.
- **제안**: 88-91행 실패 분기에 `UE_LOG(Warning)` 한 줄씩을 남긴다(모듈 관례대로 `DEFINE_LOG_CATEGORY_STATIC`). 어빌리티 사전 검증이 정상 경로를 막고 있으므로 게임플레이로 되돌릴 필요는 없고 관측만 되면 충분하다.
- **확신도**: 중간 (정상 플레이에서는 도달 빈도가 낮다 — 의도된 침묵일 수 있다)

### 6. 🟢 소모형 아이템 인스턴스를 GE `SourceObject` 로 실어, 지속형 효과에서는 나중에 널이 된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:536-540`, `:555`, `:560-563`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h:19`
- **범주**: 버그/정확성
- **문제**: 538행이 슬롯 인스턴스를 SourceObject 로 싣는데, 엔진의 `FGameplayEffectContext::SourceObject` 는 `TWeakObjectPtr` 다(엔진 `GameplayEffectTypes.h:443`). 비충전 소비 아이템은 마지막 1개가 차감되면(555행) 엔트리 제거로 인스턴스를 붙드는 강참조가 사라져 다음 GC 에 수거되므로, 즉발 GE 는 무사하지만 지속형·무한 GE 를 이후 시점에 다루는 MMC·GameplayCue 는 `GetSourceObject()` 로 널을 받는다. 차감(555행)과 적용(562행)의 순서를 바꿔도 인스턴스 수명은 같아 해결되지 않는다. 헤더(`WxItemInstance.h:19`)는 SourceObject 를 효과 측이 인스턴스별 데이터에 접근하는 진입점으로 광고하지만, 저장소에 `GetSourceObject` 호출부가 0건이고 소비 아이템이 인스턴스가 남는 충전형 하나뿐이라 아직 발현되지 않는다.
- **제안**: 헤더 계약을 "적용 시점에만 유효"로 좁히거나, 소모형 경로는 수명이 보장되는 `ItemDef` 를 SourceObject 로 싣는다. 인스턴스별 값이 필요하면 적용 시점에 Spec(SetByCaller 등)으로 복사한다.
- **확신도**: 중간

### 7. 🟢 FastArray 수신 콜백이 유효성 검사보다 먼저 `LastObservedCount` 를 갱신해, 참조 매핑 후 재호출에서 통지를 잃는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:82-88`, `:103-106`, `:59`, `:592`
- **범주**: 버그/정확성
- **문제**: `PostReplicatedAdd` 는 82행에서 `LastObservedCount` 를 무조건 갱신한 뒤 84행에서야 `Entry.Instance` 를 검사하고, 87행은 `GetItemDef()` 가 널이면 592행에서 조용히 반환된다(103-104행도 같은 형태). 인스턴스 참조가 미해결인 채 도착하면 통지는 건너뛰었는데 관찰값만 최신이 되어, GUID 매핑 후 엔진이 다시 부르는 `PostReplicatedChange`(엔진 `FastArraySerializer.h:1343-1385`)에서 `Delta == 0` 으로 106행에 걸러진다. 목록 표시는 `PostReplicatedReceive`(119-126행)가 따라잡으므로 잃는 것은 해당 아이템의 획득 연출뿐이다. 현재 설정에서는 빈도가 낮다 — 엔진이 컴포넌트의 등록 서브오브젝트를 컴포넌트 본체보다 먼저 쓰고(엔진 `DataChannel.cpp:4347-4381`) 프로젝트 설정에 비동기 넷 로딩이 없어, 미해결 도착은 드물다.
- **제안**: `LastObservedCount` 갱신을 실제 발행 분기(Instance·ItemDef 유효 확인 이후) 안으로 옮긴다. 이때 한 번도 관찰하지 못한 엔트리는 `INDEX_NONE`(-1)으로 남으므로 103행과 59행의 델타 계산에서 `INDEX_NONE` 을 0 으로 취급해야 ±1 오차가 생기지 않는다.
- **확신도**: 낮음 (복제 시스템이 Iris 로 선택되는지 확인하지 못해, 그 경우의 빈도는 판단하지 않았다)

### 8. 🟢 `RemoveItemInstance` 는 호출부 0건인 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h:175-179`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:352-384`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체에서 호출부가 0건이고 헤더 177행도 "현재 호출부가 없다"고 적는다. 33행 분량이 "슬롯 제거 + 등록 해제 + 통지" 순서를 소비 경로(`ConsumeItemsByDefinition`, 386-414행)와 따로 구현하고 있어, 발견 1 처럼 통지 규약을 바꿀 때 함께 고쳐야 하는 두 번째 갈래로 남는다.
- **제안**: 버리기·장비 해제 같은 사용처가 생길 때 다시 만들기로 하고 걷어낸다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxItemUseComponent.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`
- **훑은 파일**: `Plugins/WxInventory/README.md`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, 나머지 소스 14파일(`WxItemPickup.h`, `WxRewardLibrary.h`, `WxItemDefinition.h/.cpp`, `WxRewardTableRow.h/.cpp`, `WxStateTreeTask_GiveRewards.h/.cpp`, `WxStateTreeTask_RefillItemCharges.h/.cpp`, `WxAnimNotify_UseItem.h/.cpp`, `WxInventoryModule.h/.cpp`). 계약 교차 확인으로 `Source/WxGame/MVVM/WxViewModel_Inventory.h/.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, 엔진 `FastArraySerializer.h`·`DataChannel.cpp`·`ActorReplication.cpp`·`GameplayEffectTypes.h`·`UEBuildTarget.cs`
- **미검토 / 한계**:
  - 발견에서 제외한 확인 결과: `AddItemDefinition` 머지 루프(285-309행)가 브로드캐스트 너머로 잡는 것은 `TArray` 객체 참조라 버퍼 재할당에도 유효하고 인덱스·잔여 수량은 매 회전 다시 읽으므로, 구독자가 재진입해도 최악은 슬롯 채움 순서가 비최적이 되는 정도다. `PostReplicatedAdd/Change` 는 삭제 원소가 실제로 빠지기 전에 호출되므로(엔진 `FastArraySerializer.h:1161-1193`) 63행의 `StackCount = 0` 선반영이 합계를 맞추고, 한 번에 여러 슬롯이 지워질 때의 중간 과대 계상은 마지막 통지로 수렴한다.
  - 클라이언트는 삭제를 `RemoveAtSwap`(엔진 `FastArraySerializer.h:1193`)으로 적용해 엔트리 순서가 서버와 달라질 수 있어, `FindFirstItemStackByDefinition` 의 결과가 서버 선택과 다를 수 있다. 클라는 표시만 하고 충전형 인스턴스가 하나뿐이라 발견으로 올리지 않았다.
  - StateTree 태스크 2종이 대상 인벤토리를 0번 PlayerController 로 고정한다(`WxStateTreeTask_GiveRewards.cpp:40`, `WxStateTreeTask_RefillItemCharges.cpp:36`). 적 드랍도 같은 정책을 명시하고 있어(`Source/WxGame/Character/WxEnemyCharacter.cpp:175`) 멀티플레이 정책 미결로 보고 제외했다. 픽업(`WxItemPickup.cpp:88-90`)·사용(`WxItemUseComponent.cpp:86-88`)은 당사자를 집으므로 정책을 정할 때 두 태스크의 인스턴스 데이터에 바인딩 가능한 대상 액터를 함께 노출해야 한다.
  - `AWxItemPickup::OnInteracted`·`SetItemDef` 에는 `HasAuthority()` 게이트가 없지만 호출 경로 `UWxAbility_Interact` 가 `ServerOnly` + `HasAuthority` 로 막고 있어(`WxAbility_Interact.cpp:18`, `:49`) 제외했다. 클라 경로가 생기면 Shipping 에서는 `AddItemDefinition` 의 `check`(275행)가 컴파일 아웃되어 로컬 상태만 조용히 어긋난다.
  - 소비 성립은 서버 측 AnimNotify 발화에 걸려 있다. `UWxAbilitySystemComponent::PlayMontage` 가 몽타주 동안 메시 틱 옵션을 `AlwaysTickPoseAndRefreshBones` 로 올리므로(`WxAbilitySystemComponent.cpp:25-28`, `:92-94`) 데디케이티드 서버에서도 노티파이가 뜰 조건은 코드상 갖춰져 있으나 실측하지 않았다.
  - `.uplugin` 에 `Niagara` 가 없지만 `GameplayAbilities.uplugin` 이 `Niagara` 를 의존해 UBT 의 전이 의존 계산(엔진 `UEBuildTarget.cs:5979`)에 포함되므로 경고가 나지 않는다. 발견은 아니며, GAS 쪽 의존이 바뀌면 드러나는 암묵 의존이다.
  - 이 모듈에는 세이브/복원 경로가 없다(인벤토리는 PlayerController 수명 동안만 유지되고, 새 컨트롤러는 `StartingItems` 를 다시 받는다 — 229-232행). 기능 범위의 문제라 결함으로 다루지 않았다.
  - 복제 시스템이 런타임에 Generic 과 Iris 중 무엇으로 선택되는지 확정하지 못했다(프로젝트 설정에 명시 없음). 발견 7 의 빈도 판단은 Generic 기준이다.
  - 아이템 정의·보상 DataTable 의 실제 값(`MaxStack`, Fragment 조합, Usable Effect 의 지속 정책)과 BP/WBP 내부(토스트 위젯, 사용 몽타주의 노티파이 배치)는 열지 않았다. 발견 3·4·5·6 의 발현 여부는 그 데이터에 달려 있다. 자동화 테스트는 없고, 빌드·에디터 실행은 하지 않았다.

---
*문서 기준 커밋 `9d8cb2dd` · 리뷰일 2026-09-14 · 소스 24파일 — `/module-review`로 갱신*
