# Inventory 시스템 분석: Lyra vs Wx

본 문서는 Lyra Sample Project 의 인벤토리 시스템과 본 프로젝트(Wx) 의 인벤토리 시스템을 비교 분석한다. "우리 시스템이 Lyra 의 어떤 부분을 그대로 따랐고, 어디서 의도적으로 갈라졌는가" 를 명확히 한다.

최종 갱신: 2026-05-07 — 카테고리 enum 분리, Fragment 기능 축 리네임(Equippable/Usable), Currency Fragment 삭제, Stackable Fragment + 머지 정책 도입, 인벤토리 컴포넌트 PlayerState → PlayerController 이동, **GameplayTagStackContainer 미사용으로 제거** 반영

---

## 1. 한눈에 보는 공통점·차이점

| 영역 | Lyra | Wx | 동일성 |
|---|---|---|---|
| **ItemDefinition 타입** | `UCLASS(Blueprintable, Const, Abstract)` UObject CDO | `UPrimaryDataAsset` 데이터 자산 인스턴스 | ★★☆☆☆ |
| **Definition 참조 방식** | `TSubclassOf<ULyraInventoryItemDefinition>` | `TObjectPtr<UWxItemDefinition>` | ★★☆☆☆ |
| **Fragment 모델** | `UCLASS(DefaultToInstanced, EditInlineNew, Abstract)` UObject + virtual `OnInstanceCreated` | 동일 | ★★★★★ |
| **ItemInstance** | `UObject` + `FGameplayTagStackContainer StatTags` | `UObject` + `TObjectPtr<const UWxItemDefinition>` (StatTags 미보유 — 인스턴스별 가변 상태가 필요해질 때 도입) | ★★★☆☆ |
| **InventoryEntry** | `FFastArraySerializerItem` + `LastObservedCount` 패턴 | 동일 | ★★★★★ |
| **InventoryList** | `FFastArraySerializer` + Pre/Post Replicated 콜백 | 동일 | ★★★★★ |
| **NetDeltaSerializer** | `FFastArrayDeltaSerialize` 표준 | 동일 | ★★★★★ |
| **SubObject 복제** | Push-model(`AddReplicatedSubObject`) + 레거시(`ReplicateSubobjects`) 듀얼 | 동일 | ★★★★★ |
| **GameplayTagStackContainer** | `FGameplayTagStack` + `FGameplayTagStackContainer` (FastArray) | 미사용 (호출자 부재로 제거 — Charges/내구도 도입 시 재추가) | ★☆☆☆☆ |
| **머지 정책** | 머지 없음, `AddItemDefinition` 1회 = 신규 엔트리 | `UWxItemFragment_Stackable(MaxStack)` 기반 — 부착 시 한도까지 머지, 부재 시 1슬롯=1개 | ★★★☆☆ |
| **Fragment 디스패치** | `OnInstanceCreated(Instance)` 가상 호출 | 동일 | ★★★★★ |
| **변경 알림 채널** | `UGameplayMessageSubsystem` 태그 브로드캐스트 | C++ 멀티캐스트 델리게이트 | ★★☆☆☆ |
| **알림 페이로드** | `FLyraInventoryChangeMessage` 구조체 | 멀티캐스트 파라미터 직접 전달 | ★★★☆☆ |
| **Currency 시스템** | 별도 Fragment 없음, ItemDef 자체가 키 | 동일. `EWxItemCategory::Currency` enum 값으로 분류, 별도 마커 Fragment 없음 | ★★★★★ |
| **카테고리 분류** | (Tab 분류는 위젯 측에서 별도 처리, ItemDef 에는 명시 필드 없음) | `UWxItemDefinition::Category(EWxItemCategory)` 명시 필드. UI/기능 분기 1차 축 | ★★☆☆☆ |
| **장비 시스템** | `EquipmentManagerComponent` + `EquipmentDefinition` + `EquipmentInstance` 풀 스택, `InventoryFragment_EquippableItem` 브리지 | 단순 `UWxEquipmentComponent` (단일 무기 슬롯, AbilitySet 통합 없음) | ★★☆☆☆ |
| **Pickup 모델** | `IPickupable` 인터페이스 + `FInventoryPickup`(Templates/Instances) | 단순 `AWxItemPickup` 액터(ItemDef 1개 + GrantCount) | ★★☆☆☆ |
| **MVVM 통합** | Lyra 자체에는 MVVM 미사용(전용 위젯 컨트롤러) | UE5 MVVM ViewModel 기반(`UWxViewModel_Inventory`/`UWxViewModel_Item`) | ★☆☆☆☆ |
| **인벤토리 컴포넌트 위치** | `Controller` (PlayerController) | 동일 | ★★★★★ |

---

## 2. 동일하게 가져온 부분 (상세)

### 2.1 Fragment: UObject + virtual OnInstanceCreated

#### Lyra
```cpp
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class LYRAGAME_API ULyraInventoryItemFragment : public UObject
{
public:
    virtual void OnInstanceCreated(ULyraInventoryItemInstance* Instance) const {}
};
```

#### Wx
```cpp
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class WXINVENTORY_API UWxItemFragment : public UObject
{
public:
    virtual void OnInstanceCreated(UWxItemInstance* Instance) const;
};
```

**완전히 동일.** Fragment 의 인스턴스가 Definition 안에 EditInline 으로 살아있고, `AddItemDefinition` 직후 Fragment 의 가상 함수가 새 Instance 의 초기 상태를 주입하는 진입점 역할.

### 2.2 ItemInstance: ItemDef 참조 + 슬롯 식별

#### Lyra
```cpp
UCLASS(BlueprintType)
class ULyraInventoryItemInstance : public UObject
{
public:
    void AddStatTagStack(FGameplayTag Tag, int32 StackCount);
    int32 GetStatTagStackCount(FGameplayTag Tag) const;
    TSubclassOf<ULyraInventoryItemDefinition> GetItemDef() const;
    template<typename T> const T* FindFragmentByClass() const;

private:
    UPROPERTY(Replicated) FGameplayTagStackContainer StatTags;
    UPROPERTY(Replicated) TSubclassOf<ULyraInventoryItemDefinition> ItemDef;
};
```

#### Wx
```cpp
UCLASS(BlueprintType)
class WXINVENTORY_API UWxItemInstance : public UObject
{
public:
    const UWxItemDefinition* GetItemDef() const;
    template <typename T> const T* FindFragmentByClass() const;

private:
    UPROPERTY(Replicated) TObjectPtr<const UWxItemDefinition> ItemDef;
};
```

**개념적으로 동일하나, 인스턴스별 가변 상태(StatTags)는 일단 미보유.** 현재 게임 디자인에 인스턴스별 카운터(탄약/강화/내구도/Charges) 가 등장하지 않아 dead infrastructure 제거. 그런 상태가 등장하는 순간 `FWxGameplayTagStackContainer` 를 다시 도입하거나 구체적인 UPROPERTY 필드를 추가하면 된다.

### 2.3 InventoryList: FFastArraySerializer + LastObservedCount

#### Lyra
```cpp
USTRUCT(BlueprintType)
struct FLyraInventoryEntry : public FFastArraySerializerItem
{
    UPROPERTY() TObjectPtr<ULyraInventoryItemInstance> Instance = nullptr;
    UPROPERTY() int32 StackCount = 0;
    UPROPERTY(NotReplicated) int32 LastObservedCount = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FLyraInventoryList : public FFastArraySerializer
{
    void PreReplicatedRemove(const TArrayView<int32>, int32);
    void PostReplicatedAdd(const TArrayView<int32>, int32);
    void PostReplicatedChange(const TArrayView<int32>, int32);
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);
    ULyraInventoryItemInstance* AddEntry(TSubclassOf<ULyraInventoryItemDefinition>, int32);
    void RemoveEntry(ULyraInventoryItemInstance*);
};
```

#### Wx
```cpp
USTRUCT(BlueprintType)
struct FWxInventoryEntry : public FFastArraySerializerItem
{
    UPROPERTY() TObjectPtr<UWxItemInstance> Instance;
    UPROPERTY() int32 StackCount;
    UPROPERTY(NotReplicated) int32 LastObservedCount;
};

USTRUCT(BlueprintType)
struct FWxInventoryList : public FFastArraySerializer
{
    void PreReplicatedRemove(const TArrayView<int32>, int32);
    void PostReplicatedAdd(const TArrayView<int32>, int32);
    void PostReplicatedChange(const TArrayView<int32>, int32);
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);
    UWxItemInstance* AddEntry(const UWxItemDefinition*, int32);
    void RemoveEntry(UWxItemInstance*);
};
```

**동일.** `LastObservedCount` 패턴은 클라이언트가 `PostReplicatedChange` 콜백에서 OldCount → NewCount 델타를 산출하기 위한 표준 트릭.

### 2.4 SubObject 복제 듀얼 path

양쪽 모두 두 경로를 동시 지원:

```cpp
// Push-model (UE5 권장)
void ReadyForReplication() override
{
    Super::ReadyForReplication();
    if (IsUsingRegisteredSubObjectList())
    {
        for (...) { AddReplicatedSubObject(Instance); }
    }
}

// 레거시 path (RegisteredSubObjectList 비활성 환경 안전망)
bool ReplicateSubobjects(...) override
{
    if (!IsUsingRegisteredSubObjectList())
    {
        for (...) { Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags); }
    }
}
```

**동일한 안전 패턴**. 어느 빌드 설정에서도 SubObject 가 정확히 한 번씩 복제되도록 보장.

### 2.5 Currency: ItemDef 자체가 키

Lyra 와 마찬가지로 우리도 별도 Currency Tag/Quantity 시스템을 폐지함. 골드는 `DA_Gold` 자산 1개로 표현하며, 보유 총량 조회는 `GetTotalItemCountByDefinition(GoldDef)` 로 단일화. 우리도 별도 Currency Fragment 를 두지 않고 `EWxItemCategory::Currency` 값으로 분류한다. 픽업 1개당 지급량은 `AWxItemPickup::GrantCount` 로 이동.

---

## 3. 의도적으로 갈라진 부분

### 3.1 머지 정책: Stackable Fragment 기반

**Lyra**: `AddItemDefinition` 호출 1회 = 무조건 새 엔트리. 같은 ItemDef 가 여러 슬롯에 분산되어도 합산 조회만 가능.

**Wx**: `UWxItemFragment_Stackable(MaxStack)` 부착 여부로 동작 분기.

```cpp
UWxItemInstance* AddItemDefinition(const UWxItemDefinition* ItemDef, int32 StackCount)
{
    const auto* Stackable = ItemDef->FindFragmentByClass<UWxItemFragment_Stackable>();
    const int32 MaxStack = Stackable ? Stackable->MaxStack : 1;

    // 1) 기존 엔트리에 MaxStack 한도까지 머지
    // 2) 잔여분은 새 엔트리로 MaxStack 단위 분할
}
```

| Stackable Fragment | 동작 |
|---|---|
| 부착됨 (포션, 골드, 재료 등) | 같은 ItemDef 기존 슬롯에 MaxStack 한도까지 누적, 초과분 새 슬롯 |
| 부재 (장비) | 항상 1슬롯 = 1개. 인스턴스별 가변 상태(강화/내구도) 분리 보존 |

**왜 갈랐나**
- UI 슬롯 폭증 회피 — 골드 픽업 매번 새 슬롯이 생기면 인벤이 무의미하게 길어짐
- 명시적 표현 — "스택 가능"을 데이터 어셋이 직접 선언, 인스턴스별 상태 보존 의도가 코드에 반영됨

**Lyra 와의 본질 차이**: Lyra 는 슈터 베이스라 무기 인스턴스 분리가 절대 원칙이었지만, 우리 게임은 "장비는 분리, 소비/통화는 머지" 가 양립한다. Stackable Fragment 는 그 두 정책을 하나의 데이터 표현으로 통합한 결과.

### 3.2 카테고리: Definition 의 명시 필드

**Lyra**: ItemDef 에 카테고리 enum 필드 없음. Tab 분류는 UI 위젯 측에서 Fragment 종류 등으로 별도 분기.

**Wx**: `UWxItemDefinition::Category(EWxItemCategory)` 가 명시 필드. UI/기능 분기 1차 축.

```cpp
UENUM(BlueprintType)
enum class EWxItemCategory : uint8
{
    None,
    Equipment,
    Consumable,
    Currency
};

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
EWxItemCategory Category;
```

**왜 갈랐나**
- 카테고리는 "이 아이템이 무엇인가"(분류축), Fragment 는 "이 아이템이 무엇을 할 수 있는가"(기능축) — 두 축을 분리해 같은 카테고리 안에서도 Fragment 조합으로 다양성을 살림
- Fragment 종류에서 카테고리를 도출하는 방식은 새 Fragment 추가 시 enum 동기화·우선순위 분기 누적이 필연 → 분리로 결합도 제거
- UI 분기가 한 줄(`switch(Def->Category)`) — Fragment 순회/우선순위 코드 불필요

**Fragment 와 카테고리의 역할 정리**
- `UWxItemFragment_Equippable` (메시·소켓): Equipment 카테고리에 주로 부착되지만, 클래스 자체는 카테고리에 종속되지 않음
- `UWxItemFragment_Usable` (GE 적용): Consumable 에 주로 부착, 단 일회성 버프 스크롤 등 다른 카테고리에서도 재사용 가능
- `UWxItemFragment_Stackable` (MaxStack): 카테고리 무관 — Currency 든 Consumable 이든 스택 가능 의지를 표현

### 3.3 Definition: DataAsset vs BP CDO

이번 리팩토링에서 가장 신중히 결정한 갈래.

**Lyra**
```cpp
UCLASS(Blueprintable, Const, Abstract)
class ULyraInventoryItemDefinition : public UObject
{
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
    TArray<TObjectPtr<ULyraInventoryItemFragment>> Fragments;
};
```
- 데이터 보관소: BP 클래스의 CDO(Class Default Object)
- 참조: `TSubclassOf<ULyraInventoryItemDefinition>`
- 데이터 접근: `GetDefault<ULyraInventoryItemDefinition>(DefClass)->X`

**Wx**
```cpp
UCLASS(BlueprintType)
class UWxItemDefinition : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, Instanced, Category="Item")
    TArray<TObjectPtr<UWxItemFragment>> Fragments;
};
```
- 데이터 보관소: `.uasset` 파일 (DataAsset 인스턴스)
- 참조: `TObjectPtr<UWxItemDefinition>`
- 데이터 접근: `Def->X` (직접)

**왜 갈랐나**
- **콘텐츠 워크플로우 단순함**: DataAsset 우클릭 1단계로 새 아이템 생성. BP 클래스 + CDO 입력 2단계 대비 작가 친화적
- **BP 상속 다형성을 활용할 계획 없음**: 우리 게임은 평면적 카탈로그(검 한 종, 도끼 한 종 식). `BP_Sword_Iron → BP_Sword_Iron_Magic` 같은 깊은 계층 불필요
- **GameFeature 동적 로드 미사용**: BP CDO 의 자동 lifecycle 관리 가치가 없음
- **기존 DA 자산 재사용**: `DA_Gold_500`, `DA_Katana` 등 이미 만든 콘텐츠를 그대로 활용

**대신 우리가 잃은 것**
- BP 상속을 통한 정의 다형성(자식 BP 가 부모의 Fragment 위에 추가/오버라이드)
- `Const` UCLASS 가 자동 강제하는 런타임 변조 차단(우리는 `EditDefaultsOnly` 명시로 수동 강제)
- `TSubclassOf` 의 클래스 NetGUID 기반 더 가벼운 네트워크 복제(미세한 차이, 게임플레이에는 무관)

**공통**: Fragment 자체는 Lyra 식 `UObject (DefaultToInstanced, EditInlineNew)` 그대로 유지. Definition 안에서 EditInline 으로 추가/편집되는 방식은 동일.

### 3.4 변경 알림: GameplayMessageSubsystem 미도입

**Lyra**
```cpp
void FLyraInventoryList::BroadcastChangeMessage(FLyraInventoryEntry& Entry, int32 OldCount, int32 NewCount)
{
    FLyraInventoryChangeMessage Message;
    Message.InventoryOwner = OwnerComponent;
    Message.Instance = Entry.Instance;
    Message.NewCount = NewCount;
    Message.Delta = NewCount - OldCount;

    UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(...);
    MessageSystem.BroadcastMessage(TAG_Lyra_Inventory_Message_StackChanged, Message);
}
```

**Wx**
```cpp
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventoryStackChanged,
    const UWxItemDefinition*, int32 /*NewCount*/, int32 /*Delta*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventorySlotChanged,
    UWxItemInstance*, int32 /*NewStackCount*/, int32 /*Delta*/);
```

**왜 갈랐나**
- `GameplayMessageRouter` 플러그인 미사용 — 의존성 추가 회피
- 우리 ViewModel 이 이미 매니저 직접 참조로 동작 중. 메시지 시스템으로 갈아타는 추가 가치(디커플링)는 현 단계에서 우선순위 낮음
- 향후 listener 가 다양화되면(Sound/Toast/Achievement 등) 메시지 시스템으로 전환 검토

**대신 우리가 잃은 것**: ViewModel ↔ 매니저 완전 디커플링, 글로벌 listener 등록 편의성. 얻은 것: 의존성 단순함, 명확한 강한 타입 페이로드.

### 3.5 장비 시스템: 단순화 유지

**Lyra**: `ULyraEquipmentManagerComponent` (Pawn) + `ULyraEquipmentDefinition` (BP CDO, AbilitySet/InstanceClass 보관) + `ULyraEquipmentInstance` (UObject, virtual OnEquipped/OnUnequipped, ActorsToSpawn) + `UInventoryFragment_EquippableItem` (인벤→장비 브리지)

**Wx**: `UWxEquipmentComponent` (Pawn) — 단일 무기 액터 슬롯 + 메시 스왑

**왜 갈랐나**
- 게임 디자인상 단일 무기 슬롯, 인스턴스별 가변 어빌리티/이펙트 부여 필요 없음
- AbilitySet 통합·다중 슬롯·장비별 액터 스폰 같은 Lyra 풀 스택의 가치가 단일 무기 게임에 비해 과도

**연결 방식**: `UWxItemFragment_Equippable`(SkeletalMesh, AttachSocket) 이 인벤→장비 브리지 역할. `UWxInventoryManagerComponent::EquipItemByDef(const UWxItemDefinition*)` 가 소유 폰의 `UWxEquipmentComponent`(WxInventory)를 찾아 `EquipItem` 호출. 외형은 `OnEquipVisualChanged`(USkeletalMesh*, FName) 델리게이트로 게임 측에 방송. Lyra 의 `InventoryFragment_EquippableItem` 패턴과 컨셉은 같지만, 위임처가 EquipmentDefinition CDO 가 아니라 Wx Fragment 자체.

### 3.6 Pickup 모델: 단순화 유지

**Lyra**: `IPickupable` 인터페이스 + `FInventoryPickup`(`Templates`/`Instances` 두 페이로드) — 신규 인스턴스 생성용 ItemDef 와 기존 인스턴스 회수용 Instance 를 동시 노출

**Wx**: `AWxItemPickup` — `TObjectPtr<UWxItemDefinition> ItemDef` + `int32 GrantCount` 단일 형태

**왜 갈랐나**
- 던진 무기 회수, 적 시체 다중 아이템 루팅 같은 시나리오 미지원 → Templates/Instances 분리 가치 없음
- 보물상자도 ST 의 `Grant Reward` 태스크가 보상 행 하나를 지급하는 단순 모델

향후 무기 드롭/회수가 필요해지면 Lyra 의 IPickupable + FInventoryPickup 모델을 도입 가능. 현재는 의도적 미도입.

### 3.7 MVVM 통합

**Lyra**: 자체 위젯 컨트롤러 패턴(MVVM 미사용)

**Wx**: UE5 표준 MVVM ViewModel 기반
- `UWxViewModel_Inventory` (싱글톤 Shell, GlobalCollection 등록)
- `UWxViewModel_Item` (슬롯/Def 두 모드)
- `UWxViewModelResolver_Item` (WBP View Bindings Resolver)

이는 Lyra 와 무관한 우리 프로젝트의 별도 표준. 매니저의 델리게이트를 ViewModel 이 구독해서 `FieldNotify` 프로퍼티로 변환 → UMG 가 일반 바인딩으로 소비. 매니저 자체는 MVVM 무지(無知), MVVM 레이어가 매니저를 데이터 소스로 사용.

### 3.8 인벤토리 컴포넌트 위치

**Lyra/Wx 모두 `PlayerController`**. Pawn 라이프사이클과 디커플링되며, 소유 클라이언트 단위로만 복제되어 네트워크 효율적이다. ViewModel 초기화는 `ReceivedPlayer` 단일 경로에서 수행한다.

이전에는 PlayerState 에 두었으나(타 플레이어 인벤 조회 가능성), 4인 co-op 액션 RPG 의 실 사용 패턴에서 동료 인벤 inspect/공유가 핵심 기능이 아니라 판단 — 트레이드/공유는 명시적 RPC 시점에 페이로드를 받는 것으로 충분하고, 평소엔 본인 인벤만 복제하는 게 효율적이다.

---

## 4. 현재 우리 시스템 핵심 구성

```
WxInventory (Plugin)
├── Items/
│   ├── WxItemFragment              (UObject base + Equippable/Usable/Stackable)
│   ├── WxItemDefinition            (UPrimaryDataAsset, Category enum + Fragment 배열)
│   └── WxItemInstance              (UObject, TObjectPtr<const UWxItemDefinition> 만 보유)
└── Inventory/
    ├── WxEquipmentComponent        (단일 무기 슬롯: EquippedItemDef 복제 + EquipEffect GE 수명, OnEquipVisualChanged 방송)
    └── WxInventoryManagerComponent (FastArray + 듀얼 SubObject 복제 + Stackable 머지)

WxGame (Module)
├── Character/WxCharacterBase       (OnEquipVisualChanged 바인딩 → 무기 메시 스왑/소켓 재부착)
├── WorldObject/
│   ├── WxItemPickup                (TObjectPtr<UWxItemDefinition> + GrantCount)
│   └── WxTreasureChest             (TObjectPtr<UWxItemDefinition> 스폰)
└── MVVM/
    ├── WxViewModel_Inventory       (싱글톤, AllItems/CategorizedItems/LastChanged)
    └── WxViewModel_Item            (슬롯/Def 두 모드, Resolver 지원)
```

---

## 5. API 매핑표

| Lyra 메서드 | Wx 메서드 | 시그니처 동일성 |
|---|---|---|
| `AddItemDefinition(TSubclassOf<...>, int32)` | `AddItemDefinition(const UWxItemDefinition*, int32)` | ★★★☆☆ (DA 모델로 인한 타입 차이 + Stackable 기반 머지 로직 차이) |
| `AddItemInstance(Instance*)` | `AddItemInstance(Instance*, int32 StackCount)` | ★★★★☆ (StackCount 명시 인자 추가) |
| `RemoveItemInstance(Instance*)` | `RemoveItemInstance(Instance*)` | ★★★★★ |
| `ConsumeItemsByDefinition(TSubclassOf<...>, int32)` | `ConsumeItemsByDefinition(const UWxItemDefinition*, int32)` | ★★★★☆ (시그니처 거의 동일, 내부 동작은 우리는 partial-stack 차감 지원) |
| `FindFirstItemStackByDefinition(TSubclassOf<...>)` | `FindFirstItemStackByDefinition(const UWxItemDefinition*)` | ★★★★☆ |
| `GetTotalItemCountByDefinition(TSubclassOf<...>)` | `GetTotalItemCountByDefinition(const UWxItemDefinition*)` | ★★★★☆ |
| `GetAllItems()` | `GetAllItems()` | ★★★★★ |
| (없음) | `GetStackCountByInstance(const Instance*)` | Wx 전용 (ViewModel 슬롯 모드 초기화용) |
| (Lyra Equipment 컴포넌트) | `UseItemByDef(const UWxItemDefinition*)` | Wx 전용 (Consumable + GE 적용) |
| (Lyra Equipment 컴포넌트) | `EquipItemByDef(const UWxItemDefinition*)` | Wx 전용 (소유 폰의 UWxEquipmentComponent 에 위임) |

---

## 6. 의사 결정 이력

| 결정 | 선택 | 근거 |
|---|---|---|
| Definition: DataAsset vs BP CDO | **DataAsset** | 콘텐츠 워크플로우 단순함, BP 상속/GameFeature 미사용, 기존 자산 재사용 |
| Fragment: USTRUCT → UObject | **UObject** | virtual `OnInstanceCreated` 동작 주입, 향후 Fragment 종류 확장 시 외부 디스패치 코드 누적 회피 |
| 머지 정책: 항상 신규 → Stackable Fragment 기반 | **Stackable 기반** | 슬롯 폭증 회피(골드/포션) + 인스턴스별 상태 보존(장비) 양립. Fragment 부재 시 1슬롯=1개로 fallback |
| 카테고리: Fragment 종류에서 도출 → Definition 명시 필드 | **명시 필드** | Fragment 추가 시 enum/우선순위 동기화 결합 제거. UI 분기 한 줄로 단순화. Fragment 는 기능 축으로만 책임 |
| 알림: 멀티캐스트 → GameplayMessage | **멀티캐스트 유지** | 의존성 단순화, 현재 listener 수가 적음 |
| Equipment: 풀 Lyra 스택 | **단순화 유지** | 단일 무기 슬롯 게임에 EquipmentDefinition/Instance 3중 추상화 과함 |
| Pickup: IPickupable + Templates/Instances | **단순화 유지** | 던진 무기 회수 등 시나리오 미사용 |
| Currency: 별도 Fragment(CurrencyTag/Quantity) | **폐지 → ItemDef 자체가 키, Category enum 으로 분류** | Lyra 일관성, GrantCount 는 Pickup 액터로 이동, 마커 Fragment 도 enum 으로 흡수 |
| Fragment 마커 vs 기능 분리 | **기능 축으로 통일 (Equippable/Usable/Stackable)** | `_Equipment/_Consumable/_Currency` 마커 명을 행동 중심으로 리네임/삭제. 카테고리는 enum 책임 |

---

## 7. 향후 확장 시 검토 포인트

1. **`UWxItemFragment_Charges`** — 다크소울 에스트형(인스턴스 단위 충전, 사용 시 인벤 차감 X, 체크포인트에서 Max 리셋). 도입 시 `FWxGameplayTagStackContainer` 인프라를 ItemInstance 에 다시 추가하고 `OnInstanceCreated` 에서 InitialCharges 를 주입, `UseItemByDef` 가 Stackable 차감 대신 charge 차감 분기 추가 필요
2. **`UWxItemFragment_StatModifier`** — 장비/버프 소비템 공용 어트리뷰트 모디파이어. GAS Modifier 적용 시점(장착 시/사용 시) 분기 필요
3. **`UWxItemFragment_AbilityGrant`** — 장착·소지 시 부여 어빌리티(무기 스킬, 세트 효과). `UWxEquipmentComponent::EquipItem` 후 ASC 에 GiveAbility, 해제 시 Clear
4. **GameplayMessageSubsystem 도입** — listener 다양화(Toast/Sound/Achievement) 시
5. **Equipment 풀 스택** — 다중 슬롯, 장비별 AbilitySet 부여 시 `UWxItemFragment_AbilityGrant` 와 함께 검토
6. **IPickupable 인터페이스** — 던진 무기 회수, 다중 아이템 드랍 시
7. **Definition: DataAsset → BP CDO 전환** — BP 상속 계층(부모 BP 의 Fragment 를 자식이 확장) 활용 의지가 생길 때

---

## 8. 참고 자료

- [Lyra Inventory and Equipment in Unreal Engine — UE 5.7 Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-inventory-and-equipment-in-unreal-engine)
- [LyraStarterGame Inventory System — X157 Dev Notes](https://x157.github.io/UE5/LyraStarterGame/Inventory/)
- [LyraStarterGame Equipment System — X157 Dev Notes](https://x157.github.io/UE5/LyraStarterGame/Equipment/)
- [Fixing Lyra's Inventory System — garashka LyraDocs](https://garashka.github.io/LyraDocs/lyra/fixing-inventory-system.html)
