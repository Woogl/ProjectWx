# WxInventory — 코드 리뷰

> 모듈 경계와 코딩 규칙은 흠이 없다. Wx 의존은 `WxCore` 하나이고, 저작권 첫 줄은 24/24, `FORCEINLINE`·`inline` 은 0건이며, 헤더 본문 정의 4건에는 모두 예외 사유 주석이 있다. 서버 권위 흐름과 FastArray 복제 골격도 건전하다. 실제로 드러나는 문제는 통지 입자도가 경로마다 달라 획득 토스트가 쪼개지는 1건이고, 나머지는 확장점·저작 데이터·실패 경로에 걸린 잠재 결함과 정리 거리다. 이전 리뷰(`9d8cb2dd`) 이후 소스 변경이 없어 이전 발견 8건을 현재 코드에서 모두 다시 확인했고(전부 유효, 라인 불변), 소스 24파일을 모두 읽었다. 인벤토리 컴포넌트·아이템 인스턴스·사용 컴포넌트·픽업·보상 경로를 정독했고, 복제 순서와 콜백 호출 순서는 UE 5.8 엔진 소스로, 소비자 계약은 `Source/WxGame` 의 뷰모델·어빌리티로 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 8 |

## 결과

### 1. 🟡 스택 변경 통지의 입자도가 경로마다 달라, 한 번의 획득이 여러 토스트로 쪼개진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:302-303`, `:318-319`, `:412`, `:66-67`, `:86-87`, `:108-109`, `:597` / 소비자 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104-122`
- **범주**: 버그/정확성
- **문제**: `OnInventoryStackChanged` 의 발행 단위가 경로마다 다르다.
  - 서버 추가 경로는 머지·분할된 슬롯마다 1회 발행한다(302-303·318-319행).
  - 서버 소비 경로는 배치 전체에 1회 발행한다(412행).
  - 클라이언트 복제 경로는 엔트리마다 1회 발행한다(66-67·86-87·108-109행).

  구독자 `UWxViewModel_Inventory::HandleStackChanged` 는 `Delta > 0` 이 올 때마다 획득 연출용 VM 을 새로 만들고, `AcquiredCount = Delta` 로 채워 `LastAcquiredItem` 에 넣는다(`WxViewModel_Inventory.cpp:110-118`). 헤더(`Source/WxGame/MVVM/WxViewModel_Inventory.h:73-74`)도 발행마다 토스트가 따로 뜨는 것을 의도로 적고 있다. 그 결과 Stackable 없는 아이템 5개를 보상으로 받으면 "x1" 이 5번 뜬다. MaxStack 99 인 아이템을 97개 든 상태에서 5개를 더 받으면 서버·스탠드얼론에서는 "x2" 와 "x3" 이 따로 뜨고, 클라이언트에서는 엔진이 Add 콜백을 먼저 부르므로(엔진 `FastArraySerializer.h:1161-1176`) "x3" 다음에 "x2" 가 뜬다. 또 발행할 때마다 597행이 엔트리 전체를 다시 합산하고, 구독자는 O(N²) 인 `RefreshAllItems()`(`WxViewModel_Inventory.cpp:121`, `:124-171`)를 돌린다. 그래서 대량 지급 한 번의 비용이 슬롯 수만큼 불어난다.
- **제안**: 발행 단위를 "변경 배치당 ItemDef 1회"로 맞춘다.
  - 서버: `AddItemDefinition` 이 델타를 누적했다가 함수 끝에서 한 번만 발행한다. 소비 경로가 이미 이 형태이고, 슬롯 단위 정보는 계속 `OnInventorySlotChanged` 가 맡는다.
  - 클라이언트: 한 번의 수신에서 Add·Change 콜백이 따로 호출된다. 그러니 세 콜백에서는 ItemDef 별 델타만 모으고, 콜백들 뒤에 호출되는 `PostReplicatedReceive`(119-126행, 엔진 `FastArraySerializer.h:1613-1619`)에서 한 번에 발행해야 서버와 같은 수량이 나온다.
- **확신도**: 높음

### 2. 🟢 `AddEntry` 가 가상 확장점을 호출하는 동안에도 `Entries` 원소 참조를 계속 쥐고 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:135-150`
- **범주**: 버그/정확성
- **문제**: 135행이 `Entries.AddDefaulted_GetRef()` 로 원소 참조 `NewEntry` 를 얻는다. 145행은 그 참조를 쥔 채 `Fragment->OnInstanceCreated(NewEntry.Instance)` 를 호출하고, 149-150행에서 참조를 다시 쓴다. `OnInstanceCreated` 는 `Public/Items/WxItemFragment.h:39-40` 과 README 가 기능 확장점으로 내세우는 가상 함수다. 파생 Fragment 가 그 안에서 같은 인벤토리에 아이템을 추가하거나 차감하면(묶음 아이템 개봉 등) `Entries` 버퍼가 재할당되거나 원소가 옮겨진다. 그러면 `MarkItemDirty(NewEntry)` 가 해제된 메모리에 쓴다. 현재 오버라이드는 충전량만 채우는 `UWxItemFragment_Charges` 하나라 아직 발현되지 않는다.
- **제안**: 인스턴스를 로컬 변수에서 먼저 완성한다(`NewObject` → `SetItemDef` → Fragment 루프). 그다음 `AddDefaulted_GetRef()` 로 엔트리를 만들고, 대입부터 `MarkItemDirty` 까지 중간 호출 없이 끝낸다.
- **확신도**: 중간

### 3. 🟢 `MaxStack` 이 0 이하면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:278`, `:312-330`
- **범주**: 성능/안전
- **문제**: 278행은 `Stackable->MaxStack` 을 검증 없이 쓴다. 값이 0 이면 314행의 `ChunkCount` 가 0 이 되어 325행이 `Remaining` 을 줄이지 못한다. 음수면 `Remaining` 이 오히려 늘어난다. 어느 쪽이든 while 루프가 끝나지 않고, 한 바퀴마다 엔트리와 `UWxItemInstance` 를 만들어 게임이 멈추고 메모리가 고갈된다. `ClampMin = "1"`(`Public/Items/WxItemFragment.h:98`)은 에디터 입력 위젯에만 적용된다. 클램프가 붙기 전에 저장된 값이나, 스크립트·툴셋이 리플렉션으로 쓴 값은 막지 못한다.
- **제안**: 278행을 `const int32 MaxStack = FMath::Max(1, Stackable ? Stackable->MaxStack : 1);` 로 바꿔 한 곳에서 방어한다.
- **확신도**: 중간

### 4. 🟢 Stackable 과 Charges 를 함께 붙이면 스택 전체가 충전량 하나를 공유한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:283-310`, `:547-554`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp:11-19`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h:30-64`
- **범주**: 설계/구조
- **문제**: 충전량은 `UWxItemInstance` 단위로 관리되는데, Stackable 은 여러 개를 인스턴스 하나로 머지한다. 두 Fragment 를 함께 붙인 아이템을 3개 얻는 경우를 보자.
  - 첫 획득만 인스턴스를 만들고, `OnInstanceCreated` 로 `MaxCharges` 를 채운다(`WxItemFragment.cpp:17`).
  - 이후 획득은 머지 경로(283-310행)를 타서 인스턴스를 만들지 않는다.
  - `UseItemByDef` 는 Charges 가 있으면 스택은 그대로 두고 그 인스턴스 하나의 충전량만 줄인다(547-554행).

  결국 3개를 들고도 총 `MaxCharges` 회만 쓸 수 있다. `UWxItemDefinition` 에 `IsDataValid` 가 없어 저작 시점에 경고도 나오지 않는다. 기능 축을 직교로 두는 Fragment 컴포지션 자체는 의도된 설계지만, 이 조합만은 성립하지 않는다.
- **제안**: `UWxItemDefinition::IsDataValid` 에서 Stackable 과 Charges 를 함께 붙인 정의를 에러로 잡는다. Charges 클래스 주석(`Public/Items/WxItemFragment.h:57-62`)에는 Usable 과 달리 Stackable 과는 함께 쓸 수 없다고 적는다.
- **확신도**: 중간

### 5. 🟢 `HandleUseItemEvent` 가 `UseItemByDef` 의 실패를 버려, 마시는 모션만 나가고 흔적이 남지 않는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp:83-91`
- **범주**: 버그/정확성
- **문제**: 90행은 `UseItemByDef` 의 반환값을 버리고, 88행에서 인벤토리를 찾지 못해도 조용히 끝난다. `UseItemByDef` 가 false 를 돌려주는 경로는 6개다: `WxInventoryComponent.cpp:508`(정의가 널), `:516`(Usable 없음), `:522`(사용 가능 인스턴스 없음), `:533`(대상 ASC 없음), `:543`(GE Spec 무효), `:557`(차감 실패). 어빌리티 발동 시점의 사전 검증(`Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp:35`)과 AnimNotify 시점 사이에 상태가 바뀌면, 플레이어는 몽타주를 끝까지 보고도 회복을 받지 못한다. 83-84행에서 `PendingItemDefinition` 을 이미 비웠으므로 다시 시도할 여지도 없다. 같은 모듈의 픽업·보상 실패 경로는 진단 로그를 남긴다(`WxItemPickup.cpp:93`, `WxRewardLibrary.cpp:62`·`:70`). 반면 이 파일에는 로그 카테고리조차 없어, Effect 클래스 누락 같은 저작 실수도 묻힌다.
- **제안**: 88-91행의 실패 분기마다 `UE_LOG(Warning)` 을 한 줄씩 남긴다(모듈 관례대로 `DEFINE_LOG_CATEGORY_STATIC`). 정상 경로는 어빌리티 사전 검증이 이미 막고 있으므로, 실패를 게임플레이로 되돌릴 필요 없이 관측만 되면 충분하다.
- **확신도**: 중간 (정상 플레이에서는 거의 도달하지 않는다. 의도된 침묵일 수 있다)

### 6. 🟢 소모형 아이템 인스턴스를 GE `SourceObject` 로 실어, 지속형 효과에서는 나중에 널이 된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:536-540`, `:555`, `:560-563`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h:19`
- **범주**: 버그/정확성
- **문제**: 538행이 슬롯 인스턴스를 SourceObject 로 싣는다. 그런데 엔진의 `FGameplayEffectContext::SourceObject` 는 `TWeakObjectPtr` 다(엔진 `GameplayEffectTypes.h:443`). 비충전 소비 아이템은 마지막 1개가 차감되면(555행) 엔트리가 제거되고, 인스턴스를 붙들던 강참조가 사라져 다음 GC 에 수거된다. 즉발 GE 는 영향이 없다. 하지만 지속형·무한 GE 를 나중에 다루는 MMC·GameplayCue 는 `GetSourceObject()` 로 널을 받는다. 차감(555행)과 적용(562행)의 순서를 바꿔도 인스턴스 수명은 같으므로 해결되지 않는다. 헤더(`WxItemInstance.h:19`)는 SourceObject 를 효과 쪽이 인스턴스별 데이터에 접근하는 진입점으로 소개한다. 다만 저장소에 `GetSourceObject` 호출부가 0건이고, 소비 아이템도 인스턴스가 남는 충전형 하나뿐이라 아직 발현되지 않는다.
- **제안**: 둘 중 하나를 택한다. 헤더 계약을 "적용 시점에만 유효"로 좁히거나, 소모형 경로에서는 수명이 보장되는 `ItemDef` 를 SourceObject 로 싣는다. 인스턴스별 값이 필요하면 적용 시점에 Spec(SetByCaller 등)으로 복사한다.
- **확신도**: 중간

### 7. 🟢 FastArray 수신 콜백이 유효성 검사보다 먼저 `LastObservedCount` 를 갱신해, 참조 매핑 후 재호출 때 통지를 잃는다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:82-88`, `:103-106`, `:59`, `:592`
- **범주**: 버그/정확성
- **문제**: `PostReplicatedAdd` 는 82행에서 `LastObservedCount` 를 무조건 갱신하고, 84행에서야 `Entry.Instance` 를 검사한다. 또 `GetItemDef()` 가 널이면 87행의 호출이 592행에서 조용히 반환된다. 103-104행도 같은 순서다. 따라서 인스턴스 참조가 해결되지 않은 채 엔트리가 도착하면, 통지는 건너뛰었는데 관찰값만 최신이 된다. GUID 가 매핑된 뒤 엔진이 다시 부르는 `PostReplicatedChange`(엔진 `FastArraySerializer.h:1343-1385`)에서는 `Delta == 0` 이 되어 106행에서 걸러진다. 목록 표시는 `PostReplicatedReceive`(119-126행)가 따라잡으므로, 잃는 것은 해당 아이템의 획득 연출뿐이다. 현재 설정에서는 드물다. 엔진이 컴포넌트에 등록된 서브오브젝트를 컴포넌트 본체보다 먼저 쓰기 때문이다(엔진 `DataChannel.cpp:4219-4229`).
- **제안**: `LastObservedCount` 갱신을 실제 발행 분기(Instance·ItemDef 유효 확인 이후) 안으로 옮긴다. 그러면 한 번도 관찰하지 못한 엔트리는 `INDEX_NONE`(-1)으로 남는다. 따라서 103행과 59행의 델타 계산에서 `INDEX_NONE` 을 0 으로 취급해야 ±1 오차가 생기지 않는다.
- **확신도**: 낮음 (복제 시스템이 Iris 로 선택되는지 확인하지 못해, 그 경우의 빈도는 판단하지 않았다)

### 8. 🟢 `RemoveItemInstance` 와 그 전용 하위 함수 `FWxInventoryList::RemoveEntry` 는 호출부가 없는 데드 코드다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h:175-179`, `:76-77`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:352-384`, `:153-164`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체에서 `RemoveItemInstance` 의 호출부는 0건이고, 헤더 177행도 "현재 호출부가 없다"고 적는다. `FWxInventoryList::RemoveEntry` 는 그 함수(380행)에서만 불리므로 함께 죽은 코드다. 이 두 함수, 약 45행이 "슬롯 제거 → 등록 해제 → 통지" 순서를 소비 경로(`ConsumeItemsByDefinition` 386-414행, `ConsumeByDefinition` 174-206행)와 따로 구현한다. 그래서 발견 1 처럼 통지 규약을 바꿀 때 함께 고쳐야 하는 두 번째 갈래가 된다.
- **제안**: 두 함수를 함께 걷어내고, 버리기·장비 해제 같은 사용처가 생길 때 다시 만든다.
- **확신도**: 높음

### 9. 🟢 인라인 예외 사유 주석이 지금은 없는 "코딩 규칙 4" 를 가리킨다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h:39`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h:57`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h:13`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h:12`
- **범주**: 규칙 위반
- **문제**: 네 주석 모두 "코딩 규칙 4 의 예외다"라고 적는다. 그런데 커밋 `5fe1ceb6` 에서 람다 규칙이 빠지면서 인라인 금지 규칙은 CLAUDE.md 의 3번이 되었고, 지금 CLAUDE.md 에는 4번 규칙이 없다. 예외 자체(템플릿 함수, StateTree `GetInstanceDataType()`)와 사유 문장은 규칙에 맞으며, 틀린 것은 인용한 번호뿐이다. 같은 문구가 저장소 전체 27개 헤더에 있다.
- **제안**: 번호를 "인라인 정의 금지 규칙의 예외다"처럼 규칙 이름으로 바꾼다. 그러면 규칙 번호가 다시 바뀌어도 주석이 낡지 않는다. 다른 모듈과 함께 일괄 치환한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**:
  - `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`
  - `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`
  - `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`
  - `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`
  - `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp`
  - `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxItemUseComponent.h`
  - `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`
  - `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`
  - `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`
  - `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`
  - `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`
  - `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`
- **훑은 파일**:
  - 모듈 설정·문서: `Plugins/WxInventory/README.md`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`
  - 나머지 소스 12파일: `WxItemPickup.h`, `WxRewardLibrary.h`, `WxItemDefinition.h/.cpp`, `WxRewardTableRow.h/.cpp`, `WxStateTreeTask_GiveRewards.h`, `WxStateTreeTask_RefillItemCharges.h`, `WxAnimNotify_UseItem.h/.cpp`, `WxInventoryModule.h/.cpp`
  - 계약 교차 확인(프로젝트): `Source/WxGame/MVVM/WxViewModel_Inventory.h/.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`
  - 엔진 교차 확인: `FastArraySerializer.h`, `DataChannel.cpp`, `PackageMapClient.cpp`, `UObjectGlobals.cpp`, `CoreNative.cpp`, `GameplayEffectTypes.h`, `StateTreeNodeBase.h`, `StateTreeCompiler.cpp`, `GameplayAbilities.uplugin`
- **미검토 / 한계**:
  - **다시 확인했으나 발견에서 제외한 사항**:
    - `AddItemDefinition` 머지 루프(285-309행)가 브로드캐스트 동안 쥐는 것은 `TArray` 객체 참조다. 이 참조는 버퍼 재할당에도 유효하고, 인덱스·잔여 수량은 매 회전 다시 읽는다. 따라서 구독자가 재진입해도 최악의 결과는 슬롯을 채우는 순서가 최적이 아니게 되는 정도다.
    - 엔진은 삭제 원소를 실제로 빼기 전에 `PreReplicatedRemove` 를 호출한다(엔진 `FastArraySerializer.h:1148`, `:1185-1193`). 그래서 63행의 `StackCount = 0` 선반영이 합계를 맞춘다.
    - 픽업을 거의 동시에 두 번 상호작용하는 경우를 확인했다. 먼저 처리된 RPC 가 `Destroy()` 를 부르면, 다음 RPC 의 대상은 GUID 캐시의 약참조 검사(엔진 `PackageMapClient.cpp:3760`)에서 널로 풀린다. 따라서 중복 지급으로 이어지지 않는다.
    - 생성자의 `InventoryList(this)` 는 BP 파생 컨트롤러(`Content/Framework/BP_PlayerController`)에서 템플릿 값으로 덮인다. 하지만 `UActorComponent` 가 `DefaultToInstanced` 라 인스턴싱 그래프가 라이브 컴포넌트로 다시 매핑한다(엔진 `CoreNative.cpp:433-436`, `:225-247`). 이는 Lyra 와 같은 패턴이라 제외했다.
  - **엔트리 순서**: 클라이언트는 삭제를 `RemoveAtSwap`(엔진 `FastArraySerializer.h:1193`)으로 적용하므로, 엔트리 순서가 서버와 달라질 수 있다. 그러면 `FindFirstItemStackByDefinition`·`FindUsableInstance` 의 선택이 서버와 다를 수 있다. 클라이언트는 표시와 예측 판정만 하고 최종 판정은 서버가 하므로 발견으로 올리지 않았다.
  - **보상·리필 대상 고정**: StateTree 태스크 2종은 대상 인벤토리를 0번 PlayerController 로 고정한다(`WxStateTreeTask_GiveRewards.cpp:40`, `WxStateTreeTask_RefillItemCharges.cpp:36`). 서버에서 0번은 호스트이거나 먼저 접속한 플레이어이므로, 멀티플레이에서는 한 명만 받는다. 적 드랍도 같은 정책을 명시하고 있어(`Source/WxGame/Character/WxEnemyCharacter.cpp:175`) 멀티플레이 정책이 미결인 것으로 보고 제외했다. 픽업(`WxItemPickup.cpp:88-90`)과 사용(`WxItemUseComponent.cpp:86-88`)은 당사자를 고르므로, 정책을 정할 때 두 태스크의 인스턴스 데이터에도 바인딩 가능한 대상 액터를 함께 노출해야 한다.
  - **권한 게이트**: `AWxItemPickup::OnInteracted`·`SetItemDef` 에는 `HasAuthority()` 게이트가 없다. 다만 호출 경로인 `UWxAbility_Interact` 가 `ServerOnly` 와 `HasAuthority` 로 막고 있어(`WxAbility_Interact.cpp:18`, `:49`) 제외했다. 클라이언트 호출 경로가 생기면, Shipping 에서는 `AddItemDefinition` 의 `check`(275행)가 컴파일에서 빠져 로컬 상태만 조용히 어긋난다.
  - **서버 측 AnimNotify**: 소비는 서버에서 AnimNotify 가 발화해야 성립한다. `UWxAbilitySystemComponent::PlayMontage` 가 몽타주 동안 메시 틱 옵션을 `AlwaysTickPoseAndRefreshBones` 로 올리므로(`WxAbilitySystemComponent.cpp:26-29`, `:92-94`) 데디케이티드 서버에서도 노티파이가 뜰 조건은 코드상 갖춰져 있다. 실측은 하지 않았다.
  - **Niagara 암묵 의존**: `.uplugin` 에 `Niagara` 가 없지만, `GameplayAbilities.uplugin` 이 `Niagara` 를 의존하므로 전이 의존으로 포함되어 경고가 나지 않는다. 발견으로 올리지는 않았다. GAS 쪽 의존이 바뀌면 드러나는 암묵 의존이다.
  - **빈 인스턴스 데이터 구조체**: `FWxStateTreeTask_RefillItemChargesInstanceData` 는 빈 구조체다. 엔진은 인스턴스 데이터 없는 노드를 허용하므로(엔진 `StateTreeNodeBase.h:94-97`, `StateTreeCompiler.cpp:2211`) 걷어낼 수 있다. 다만 기존 StateTree 에셋의 노드 인스턴스 데이터가 어떻게 이행되는지 확인하지 못해 제안하지 않았다.
  - **세이브/복원 없음**: 이 모듈에는 세이브/복원 경로가 없다. 인벤토리는 PlayerController 가 살아 있는 동안만 유지되고, 새 컨트롤러는 `StartingItems` 를 다시 받는다(229-232행). 기능 범위의 문제라 결함으로 다루지 않았다.
  - **복제 시스템 미확정**: 런타임에 Generic 과 Iris 중 어느 복제 시스템이 선택되는지 확정하지 못했다(프로젝트 설정에 명시가 없다). 발견 7 의 빈도 판단은 Generic 기준이다.
  - **데이터·BP 내부 미검토**: 아이템 정의·보상 DataTable 의 실제 값(`MaxStack`, Fragment 조합, Usable Effect 의 지속 정책)과 BP/WBP 내부(토스트 위젯, 사용 몽타주의 노티파이 배치)는 열지 않았다. 발견 3·4·5·6 이 실제로 발현되는지는 그 데이터에 달려 있다.
  - **테스트·빌드**: 자동화 테스트는 없고, 빌드와 에디터 실행은 하지 않았다.

---
*문서 기준 커밋 `5eb1a754` · 리뷰일 2026-09-17 · 소스 24파일 — `/module-review`로 갱신*
