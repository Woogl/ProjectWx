# WxInventory — 코드 리뷰

> 이번 검토는 `08c73f51`을 기준으로 작업 트리에 있는 인벤토리 수명 통지, 지연 복제 상태 통지, 정적 조회 헬퍼 제거에 한정한다. 이 변경에서 새로 보고할 고신호 결함은 발견하지 않았다. 모듈 전체가 결함 없다는 의미는 아니며 이전 리뷰 결과는 아래에 별도로 보존한다.

## 요약

| 심각도 | 이번 변경의 신규 발견 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 0 |

## 결과

- `BeginPlay`에서 기존 `OnAnyInventoryReady`만 발행하고 `EndPlay`에서 별도 `OnAnyInventoryEnded`를 발행한다. UE 5.8 `UActorComponent::EndPlay`가 `bHasBegunPlay`를 해제한 다음 종료 통지가 전달되므로 뷰모델의 초기 연결 검사와 맞는다. 재등록을 새로운 수명으로 취급하지 않는 현재 요구에도 맞는다.
- `FindInventory` 제거 후 픽업·보상 경로는 기존 Pawn→PlayerController 변환을 호출처에 보존한다. 시작 아이템 액션은 기존처럼 BeginPlay를 마친 컴포넌트만 초기 조회로 지급하고 Ready를 계속 구독한다.
- `FWxInventoryList::PostReplicatedReceive`와 `UWxItemInstance::HandleItemDefReplicated`가 목록 상태 재조회 통지를 제공한다. 엔진의 미해결 FastArray 참조 후속 매핑 경로에서도 전자가 호출됨을 확인했다. 두 뷰모델이 이 통지를 구독하므로 이전 리뷰 1번의 HUD 목록 영구 누락 원인은 코드상 보완되었다. 이 통지는 획득 Delta를 복구하는 이벤트가 아니며, 모든 기존 델타 통지 유실까지 해결되었다고 판단하지 않는다.

## 검토 범위

- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemInstance.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxGameFeatureAction_AddInventoryItems.cpp`.
- **훑은 파일**: `Plugins/WxInventory/README.md`, `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, 픽업·보상·충전 리필 호출처의 변경 부분. 계약 확인 목적으로 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`와 설치된 UE 5.8의 `FastArraySerializer.h`, `ActorComponent.cpp`를 교차 확인했다.
- **미검토 / 한계**: 한 PlayerController의 인벤토리 하나가 종료된 뒤 교체되는 현재 설계를 전제로 한다. 실제 네트워크 패킷 지연·NetGUID 매핑 순서, BP/WBP 내부 바인딩, 런타임 에셋 값은 실행 검증하지 않았다. 이번에는 빌드나 자동화 테스트를 재실행하지 않았다.

## 이전 리뷰 보존 기록

아래 내용은 `491dd7ec` 기준의 기존 문서를 원문 보존한 것이다. 위치·라인·개수·단정은 당시 기준이며 이번 신규 발견 수에 포함하지 않는다. 1번의 화면 상태 누락은 위에서 변경 경로를 재검토했다. 나머지 2~6번은 이번 범위에서 전체 재검증하지 않았으므로 해결 여부를 단정하지 않는다. 특히 README 관련 서술이나 호출처 수는 현재 상태와 다를 수 있다.

# WxInventory — 코드 리뷰

> 모듈 경계·코딩 규칙 준수는 흠잡을 데 없고(WxCore 외 Wx 의존 0, 저작권 헤더 22/22, 람다·FORCEINLINE·BlueprintCallable 오용 0, override Super 누락 0), Lyra 계열 FastArray 인벤토리를 충실히 따라간 구조다. 남은 문제는 복제 수신 경로의 통지 유실 가능성, 확장점을 가로지르는 참조 수명, 그리고 배선만 남은 장비 경로 세 갈래다. 이번 리뷰는 `*.Build.cs`·`.uplugin`·전체 헤더와 `WxInventoryComponent.cpp`·`WxEquipmentComponent.cpp`·`WxRewardLibrary.cpp`·`WxItemPickup.cpp` 를 정독했고, 소비자(WxGame 뷰모델·`UWxItemUseComponent`·`UWxAbility_Interact`) 쪽은 계약 검증 목적으로만 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 FastArray 콜백에서 복제 포인터가 미해결이면 인벤토리 통지가 영구 유실된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:83`, `:88`, `:105`, `:613`
- **범주**: 버그/정확성
- **문제**: `PostReplicatedAdd` 는 `Entry.LastObservedCount = Entry.StackCount;`(83행)를 **널 검사(85행)보다 먼저** 무조건 수행한다. 클라이언트가 엔트리를 받는 시점에 `Entry.Instance`(서브오브젝트) 나 `Instance->GetItemDef()`(콘텐츠 패키지의 `UWxItemDefinition` 하드 복제 참조, `WxItemInstance.h:76`) 가 아직 NetGUID 미해결이면, 통지는 건너뛰는데 `LastObservedCount` 만 최신값으로 올라간다. 포인터가 나중에 해결돼도 FastArray 는 그 항목을 다시 dirty 로 보지 않으므로 `PostReplicatedChange` 도 오지 않고, 설령 와도 `Delta == 0` 이라 107행에서 다시 걸러진다. `NotifyStackChangedFromList` 는 `ItemDef` 가 널이면 613행에서 조기 반환하므로 정의만 미해결이어도 같은 결과다. `UWxViewModel_Inventory::RefreshAllItems` 는 `OnInventoryStackChanged` 로만 다시 도는 구조라(`Source/WxGame/MVVM/WxViewModel_Inventory.cpp:115`), 해당 아이템은 같은 정의에 다음 변경이 올 때까지 HUD 에서 통째로 누락된다. `PostReplicatedChange`(105행)도 동일 패턴이다.
- **제안**: `LastObservedCount` 갱신을 실제로 통지를 발행한 경우로 한정하고(널 검사 안쪽으로 이동), 미해결이면 다음 변경에서 델타가 복구되게 둔다. 정의 참조를 더 확실히 하려면 `UWxItemInstance::ItemDef` 에 RepNotify 를 달아 해결 시점에 소유 인벤토리로 재통지하는 경로도 함께 고려한다.
- **확신도**: 중간

### 2. 🟡 `AddEntry` 가 배열 참조를 가상 확장점 호출 너머까지 붙들고 있다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:127-142`
- **범주**: 버그/정확성
- **문제**: `Entries.AddDefaulted_GetRef()` 로 얻은 `FWxInventoryEntry& NewEntry` 를 들고 있는 상태에서 133-139행이 `Fragment->OnInstanceCreated(...)` 를 호출하고, 그 뒤 141행 `MarkItemDirty(NewEntry)` 와 142행 `NewEntry.Instance` 접근이 이어진다. `OnInstanceCreated` 는 모듈이 공식 확장점으로 광고하는 가상 함수인데(`WxItemFragment.h:39`, README "확장 포인트"), 여기서 같은 인벤토리에 아이템이 추가되면 `Entries` 가 재할당되어 `NewEntry` 가 댕글링 참조가 되고 141행이 해제된 메모리에 쓴다. 현재 유일한 구현체(`UWxItemFragment_Charges`)는 충전량만 세팅해 발현되지 않지만, "인스턴스 초기 상태 주입" 용도로 프래그먼트가 하나만 늘어도 재현 가능한 형태다.
- **제안**: 인덱스(`const int32 NewIndex = Entries.AddDefaulted();`)를 잡아두고 프래그먼트 호출 이후 `Entries[NewIndex]` 로 다시 접근한다. 한 줄 변경이며 확장점 계약을 바꾸지 않는다.
- **확신도**: 중간

### 3. 🟡 장비 경로 전체가 배선만 있고, 그 비용은 전 캐릭터가 낸다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp:14`, `:34-115`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h:179`, `:234`, `Source/WxGame/Character/WxCharacterBase.cpp:47`
- **범주**: 설계/구조
- **문제**: `UWxEquipmentComponent::EquipItem` 의 유일한 호출부인 `UWxInventoryComponent::EquipItemByDef` 를 부르는 곳이 프로젝트 전체에 0건이고(헤더도 그렇게 적고 있다), `EquippedItemDef` 는 항상 널이라 `ApplyEquipEffects`/`RemoveActiveEquipEffects` 도 도달 불가다. 그런데 `WxCharacterBase.cpp:47` 이 모든 캐릭터(플레이어·적·미니언)에 이 컴포넌트를 기본 서브오브젝트로 만들고, 컴포넌트는 `SetIsReplicatedByDefault(true)`(14행)라 액터마다 복제 등록 슬롯 하나를 계속 소모한다. `UWxInventoryComponent::RemoveItemInstance`(179행)도 호출부 0건이다. 덧붙여 `Plugins/WxInventory/README.md` 는 이 컴포넌트가 `OnEquipVisualChanged` 로 메시/소켓을 방송한다고 적고 `EquippedItemDef` 를 OnRep 이라 서술하는데, 코드에는 그런 델리게이트가 없고 `EquippedItemDef` 는 순수 `Replicated` 다 — 문서가 구현을 앞질러 있어 다음 세션이 있지도 않은 방송을 찾게 된다.
- **제안**: 장비 기능을 곧 쓸 계획이 없으면 `UWxEquipmentComponent`·`EquipItemByDef`·`RemoveItemInstance`·`UWxItemFragment_Equippable` 을 걷어내고, 유지하기로 한다면 최소한 `WxCharacterBase` 의 무조건 생성을 거두고 README 의 `OnEquipVisualChanged`/OnRep 서술을 현재 코드에 맞춘다.
- **확신도**: 높음

### 4. 🟢 `MaxStack` 이 0 이하면 `AddItemDefinition` 이 무한 루프에 빠진다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:305-323`
- **범주**: 성능/안전
- **문제**: 307행 `ChunkCount = FMath::Min(MaxStack, Remaining)` 이 0 이 되면 318행의 `Remaining -= ChunkCount` 가 진전을 못 만들어 while 루프가 매 회전마다 새 엔트리와 `UWxItemInstance` 를 만들며 영원히 돈다(행 + OOM). `UWxItemFragment_Stackable::MaxStack` 의 `ClampMin = "1"`(`WxItemFragment.h:110`)은 에디터 위젯 입력만 막지, 이전에 저장된 값이나 다른 경로로 들어온 값은 막지 못한다.
- **제안**: 271행에서 `const int32 MaxStack = FMath::Max(1, Stackable ? Stackable->MaxStack : 1);` 로 한 번 방어한다.
- **확신도**: 중간

### 5. 🟢 클라이언트 수신 경로의 `NewCount`/`Delta` 의미가 서버 경로와 어긋난다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:80-90` vs `:305-323`
- **범주**: 버그/정확성
- **문제**: `PostReplicatedAdd` 는 배열이 전부 갱신된 뒤 한 번에 호출되므로, 같은 델타에 같은 `ItemDef` 엔트리 2개가 실려 오면 첫 통지부터 `NotifyStackChangedFromList` 의 `NewCount`(618행 전체 재계산)가 이미 **최종 총량**이면서 `Delta` 는 **부분값**이 된다. 서버 경로(`AddItemDefinition`)는 엔트리를 하나 만들 때마다 통지하므로 `NewCount` 가 점증한다. 즉 같은 지급이라도 서버(리슨 호스트)와 클라에서 구독자가 받는 중간 시퀀스가 다르다. 현재 소비자는 `NewCount` 만 쓰므로(`WxViewModel_Inventory.cpp:101`) 최종 표시는 맞지만, `Delta` 로 획득 연출·누적을 계산하는 구독자가 생기면 어긋난다.
- **제안**: 계약을 하나로 고정한다 — 통지 단위를 "이 변경 배치 전체"로 올려 한 번만 발행하거나, 반대로 `Delta` 도 슬롯 단위 그대로 두되 `NewCount` 의 의미(항상 사후 총량)를 델리게이트 주석에 못 박는다.
- **확신도**: 중간

### 6. 🟢 통지마다 인벤토리 전체를 재스캔하고, 스택 분할 시 통지가 증폭된다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp:611-620`, `:305-323`
- **범주**: 성능/안전
- **문제**: `NotifyStackChangedFromList` 는 발행 때마다 `GetTotalItemCountByDefinition` 으로 엔트리 전체를 훑는데(618행), `AddItemDefinition` 의 머지·분할 루프는 슬롯 하나당 이 통지를 한 번씩 낸다. 비스택 아이템을 N개 지급하면 N회 전체 스캔(O(N·Entries))에 더해 구독자 콜백이 N번 돈다 — `UWxViewModel_Inventory::HandleStackChanged` 는 매 회 `UWxViewModel_Item` 을 `NewObject` 하고 `RefreshAllItems()` 로 목록을 통째로 재구성하므로(`WxViewModel_Inventory.cpp:108`, `:115`) 증폭이 그대로 UI 로 전달된다. 인벤토리 규모가 작아 당장 문제는 아니지만 대량 지급 한 번에 몰리는 형태다.
- **제안**: `AddItemDefinition`/`ConsumeItemsByDefinition` 안에서 슬롯 단위 통지는 유지하되 정의 단위(`OnInventoryStackChanged`) 통지는 함수 끝에서 누적 델타로 1회만 발행한다(소비 경로는 이미 405행에서 그렇게 하고 있어 추가 경로만 맞추면 된다).
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Inventory/WxInventoryComponent.h`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxEquipmentComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxRewardLibrary.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemInstance.cpp`, `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h`
- **훑은 파일**: `Plugins/WxInventory/Source/WxInventory/WxInventory.Build.cs`, `Plugins/WxInventory/WxInventory.uplugin`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_RefillItemCharges.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemFragment.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxRewardTableRow.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/WxInventoryModule.cpp`, 나머지 Public 헤더 전부. 교차 확인용으로 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxCore/.../WxInteractable.h`
- **미검토 / 한계**:
  - 두 StateTree 태스크가 대상 인벤토리를 `UGameplayStatics::GetPlayerController(Owner, 0)` 로 고정하는 점은 모듈 고유 결함이 아니라 프로젝트 전반의 싱글플레이 전제(WxQuest·WxDialogue 도 동일)라 발견으로 올리지 않았다. 멀티 정책이 정해지는 시점에 이 세 곳을 함께 봐야 한다.
  - 서버 권한 API 의 `check(HasAuthority())` 계약은 Shipping 에서 컴파일 아웃되지만, 프로젝트 전반이 취하는 Lyra 식 계약이라 그대로 두고 판단하지 않았다.
  - 발견 1·5 는 실제 델타 패킷 순서에 달린 문제라 PIE 리슨 서버/데디 조합의 실측으로는 확인하지 않았고, 코드 경로 추론에 근거한다.
  - 아이템 정의·보상 DataTable 등 데이터 에셋의 실제 값(예: `MaxStack`, `ItemActorClass` 설정 여부)은 열어보지 않았다.

---
*이전 리뷰 커밋 `491dd7ec` · 리뷰일 2026-09-05 · 당시 소스 22파일*

---
*문서 기준 커밋 `08c73f51` · 리뷰일 2026-09-05 · 소스 24파일 — `/module-review`로 갱신*

