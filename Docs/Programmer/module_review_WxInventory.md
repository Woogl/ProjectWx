# WxInventory — 코드 리뷰

> 전반적으로 건강하다 — 권한 `check`, FastArray 더티 마킹, 서브오브젝트 등록/해제, 델리게이트 수렴 구조가 일관되고 규칙 위반(Copyright·BlueprintCallable·람다·플러그인 의존)은 없다. 실질적 결함은 클라이언트 복제 콜백의 **통지 시점** 하나에 집중된다. 이번 리뷰는 매니저·인스턴스·Fragment·장비·보상·픽업·ST 태스크의 cpp까지 통독했고, 복제 경로는 UE 5.8 엔진 소스(FastArraySerializer.h·DataChannel.cpp)와 WxGame 구독자(뷰모델)까지 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 1 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 클라이언트 슬롯 제거 통지가 실제 제거 "전"에 발행되어, 풀(pull) 구독자가 제거된 슬롯을 살아 있는 것으로 본다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:44-70` (`FWxInventoryList::PreReplicatedRemove`), 대조: `:379-407` (`ConsumeItemsByDefinition`), `:345-377` (`RemoveItemInstance`)
- **범주**: 버그/정확성
- **문제**: 서버 경로는 `InventoryList.ConsumeByDefinition`/`RemoveEntry`로 엔트리를 **제거한 뒤** `NotifySlotChangedFromList`/`NotifyStackChangedFromList`를 발행한다. 반면 클라이언트 경로 `PreReplicatedRemove`는 엔진이 삭제 콜백을 먼저 부르고 실제 제거는 마지막에 하므로(UE 5.8 `Engine/Source/Runtime/Net/Core/Classes/Net/Serialization/FastArraySerializer.h:1134` "Call the delete callbacks now, actually remove them at the end" → `:1148` PreReplicatedRemove → `:1165/1176` PostReplicatedAdd/Change → `:1193` RemoveAtSwap), 델리게이트가 발행되는 순간 제거 대상 엔트리와 Instance 가 `Entries` 에 그대로 남아 있다. `:64-65`에서 `StackCount` 를 0 으로 내려 총합(`GetTotalItemCountByDefinition`)은 맞춰 두었지만, `GetAllItems()`·`FindFirstItemStackByDefinition()`·`FindUsableInstance()` 같은 풀 API 는 여전히 그 Instance 를 돌려준다. 실제 소비자가 이 경로를 탄다: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:98-116` `HandleStackChanged` → `RefreshAllItems()` → `:125` `GetAllItems()` 로 목록을 재구성하므로, 클라이언트에서는 제거된 슬롯의 Item VM 이 `AllItems`/`CategorizedItems` 에 잔존(수량 0 표시)하다가 다음 인벤토리 변경 때까지 사라지지 않는다. 리슨 서버 호스트는 서버 경로라 정상이고 원격 클라이언트에서만 재현되는 불일치다. 헤더의 계약(`WxInventoryManagerComponent.h:113-116` "제거 시 0")과 README 의 "양 경로가 같은 진입점으로 수렴" 약속이 시점 면에서 깨져 있다.
- **제안**: `FWxInventoryList` 에 `PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters&)` 를 구현하고(엔진이 배열 정리 후 호출 — 같은 헤더 `:705`), `PreReplicatedRemove` 에서는 (Instance, ItemDef, Delta) 를 멤버 큐(`NotReplicated`)에 쌓기만 한 뒤 `PostReplicatedReceive` 에서 순서대로 `NotifySlotChangedFromList`/`NotifyStackChangedFromList` 를 발행한다. 이렇게 하면 서버와 동일하게 "제거 완료 후 통지" 가 되어 풀 구독자가 안전해진다. 대안(풀 API 가 `StackCount<=0` 엔트리를 걸러내기)은 계약을 암묵적으로 만들어 권하지 않는다.
- **확신도**: 높음

### 2. 🟡 StateTree 태스크가 보상/리필 대상을 0번 PlayerController 로 고정한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:41`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp:37`
- **범주**: 설계/구조
- **문제**: 두 태스크 모두 권위 측에서만 실행되면서 `UGameplayStatics::GetPlayerController(Owner, 0)` 을 대상으로 쓴다. 싱글/리슨 서버에서는 호스트 = 0번이라 맞지만, 데디케이티드 서버나 다중 클라이언트에서는 "서버 월드의 첫 컨트롤러" 가 상호작용한 플레이어와 무관하므로 비-픽업 보상(재화 등)과 에스트병 리필이 엉뚱한 플레이어에게 간다. 같은 라이브러리를 쓰는 `Source/WxGame/Character/WxEnemyCharacter.cpp:93` 은 실제 대상 컨트롤러를 넘기고 있어, ST 경로만 다중 플레이어 전제가 빠져 있다. 헤더 주석(`WxStateTreeTask_RefillItemCharges.h:21` "로컬 플레이어(0번 컨트롤러)")이 이를 명시하고 있으므로 현재 범위에서는 의도일 수 있다.
- **제안**: 장치(AWxDevice) ST 컨텍스트가 이미 상호작용자를 알고 있다면 그 액터를 인스턴스 데이터 바인딩(`TObjectPtr<AActor> Target`)으로 받아 `FindInventory(Target)` 에 넘기고, 없으면 최소한 주석에 "싱글 플레이어 전제" 를 명시한 채 두되 MP 활성화 시 체크리스트에 올린다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 장비 경로(EquipItemByDef → UWxEquipmentComponent)와 RemoveItemInstance 가 트리거 없는 데드 코드로 남아 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:584-610` (`EquipItemByDef`), `:345-377` (`RemoveItemInstance`), `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h:28-30`
- **범주**: 중복/복잡도
- **문제**: 두 API 모두 호출부 0건이며(저장소 전체 grep), 헤더 주석이 이를 "미구현" 으로 정직하게 적어 두었다. 다만 `WxEquipmentComponent.h:30` 이 스스로 지적하듯, 늦게 relevant 해진 클라이언트는 `OnRep_EquippedItemDef` 가 캐릭터 구독(`Source/WxGame/Character/WxCharacterBase.cpp:83`)보다 먼저 오면 외형 방송을 유실하는데 현재 상태를 되물을 pull API 가 없다. 경로를 살릴 때 이 결함이 그대로 켜진다.
- **제안**: 완성할지 제거할지 결정한다. 완성 시 `UWxEquipmentComponent` 에 `GetEquippedItemDef()`(또는 현재 메시/소켓 조회)를 추가해 구독 직후 한 번 당겨 오도록 하고, 당분간 안 쓸 거면 `RemoveItemInstance` 와 함께 지워 API 표면을 줄인다.
- **확신도**: 높음(데드 코드 사실) / 낮음(제거 여부는 로드맵 판단)

### 4. 🟢 클라이언트 엔트리 제거는 RemoveAtSwap 이라 서버와 슬롯 순서가 달라지고, "첫 인스턴스" 선택이 양쪽에서 어긋날 수 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:147-155` (`RemoveEntry`), `:171-188` (`ConsumeByDefinition`, `RemoveCurrent` = 순서 보존), 소비처 `:409-425` (`FindFirstItemStackByDefinition`), `:643-669` (`FindUsableInstance`)
- **범주**: 버그/정확성
- **문제**: 서버는 `RemoveAt` 계열로 순서를 보존하지만 클라이언트 FastArray 수신은 `Items.RemoveAtSwap` (`FastArraySerializer.h:1193`) 으로 지우므로, 제거가 한 번이라도 일어난 뒤에는 같은 ItemDef 의 다중 슬롯 순서가 서버/클라에서 달라진다. 현재 영향은 표시 수준 — 클라이언트 뷰모델이 `FindFirstItemStackByDefinition` 으로 고른 인스턴스의 충전량/아이콘이 서버가 `FindUsableInstance` 로 실제 사용할 인스턴스와 다를 수 있다. 충전형이 인벤토리에서 제거되지 않는 현 설계에서는 거의 발생하지 않는다.
- **제안**: 당장 고칠 필요는 없고, 슬롯 순서에 의미를 부여하는 기능(정렬 고정·퀵슬롯 인덱스 등)을 붙일 때 서버가 순서 키를 엔트리에 복제하거나 클라가 ItemDef 단위로 안정 정렬하도록 한다. 주석으로 이 엔진 동작만 남겨 두면 충분하다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 5. 🟢 StateTree 태스크 헤더의 `GetInstanceDataType()` 인라인 정의는 코딩 규칙 6 의 명시 위반이다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h:49`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h:33`
- **범주**: 규칙 위반
- **문제**: 두 헤더가 `virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }` 를 본문째 정의한다. 같은 파일 상단 주석(`:13`/`:12`)이 "옮길 본문이 없다" 며 예외로 선언했지만, `WxItemDefinition.h:60-66` 의 템플릿과 달리 이 한 줄은 cpp 로 내리는 데 기술적 제약이 없다. 엔진 StateTree 관례를 따른 것이라 의도일 수 있다.
- **제안**: 규칙을 지킬 거면 cpp 로 옮기고, 예외로 둘 거면 CLAUDE.md 쪽에 "ST 태스크 `GetInstanceDataType` 은 예외" 를 한 줄 명시해 파일마다 주석으로 변명하지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryManagerComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxEquipmentComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`
- **훑은 파일**: `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxRewardTableRow.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_GiveRewards.h`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxStateTreeTask_RefillItemCharges.h`, `Plugins/WxInventory/Source/WxInventory/Public/WxInventoryModule.h`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`; 외부 대조: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, UE 5.8 `FastArraySerializer.h`·`DataChannel.cpp`
- **미검토 / 한계**: 정적 분석만 수행했고 PIE 클라이언트에서 1번 항목을 실제 재현하지는 않았다. WxGame 측 `WxAbility_UseItem`·`WxGameMode::GrantItems` 호출 맥락은 grep 으로 권한 경로만 확인했다. BP/DataTable 에셋(ItemDef·보상 로우·픽업 BP)은 범위 밖이다.

---
*문서 기준 커밋 `bd689a19` · 리뷰일 2026-08-22 · 소스 22파일 — `/module-review`로 갱신*
