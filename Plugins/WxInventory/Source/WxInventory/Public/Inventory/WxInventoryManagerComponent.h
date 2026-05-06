// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "WxInventoryManagerComponent.generated.h"

class UActorChannel;
class UWxItemDefinition;
class UWxItemInstance;
class UWxInventoryManagerComponent;
struct FWxInventoryList;

/**
 * 인벤토리 한 슬롯의 엔트리. 같은 ItemDef 는 매니저의 머지 정책에 따라 MaxCounts 한도 내에서 동일 엔트리에 누적되며, 초과분은 새 엔트리로 분할된다.
 */
USTRUCT(BlueprintType)
struct FWxInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FWxInventoryEntry();

	UWxItemInstance* GetInstance() const;
	int32 GetStackCount() const;

	/** StackCount에 Delta를 더한다. 음수면 차감. */
	void AddStack(int32 Delta);

	/** 신규 엔트리 초기화 — Instance/StackCount/LastObservedCount를 일관되게 설정한다. */
	void Initialize(UWxItemInstance* InInstance, int32 InStackCount);

private:
	friend FWxInventoryList;

	UPROPERTY()
	TObjectPtr<UWxItemInstance> Instance;

	UPROPERTY()
	int32 StackCount;

	/** 클라이언트 델타 계산용. 레플리케이션 대상 아님. */
	UPROPERTY(NotReplicated)
	int32 LastObservedCount;
};

/**
 * 인벤토리 엔트리 컬렉션. FastArray 로 효율 레플리케이션.
 */
USTRUCT(BlueprintType)
struct FWxInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FWxInventoryList();

	explicit FWxInventoryList(UActorComponent* InOwnerComponent);

	//~ Begin FFastArraySerializer
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~ End FFastArraySerializer

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);

	const TArray<FWxInventoryEntry>& GetEntries() const;
	TArray<FWxInventoryEntry>& GetEntriesMutable();

private:
	UPROPERTY()
	TArray<FWxInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FWxInventoryList> : public TStructOpsTypeTraitsBase2<FWxInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * 아이템 추가 결과. 단일 요청이 머지/분할로 복수 슬롯을 만질 수 있어 리스트로 반환한다.
 */
USTRUCT(BlueprintType)
struct FWxAddItemResult
{
	GENERATED_BODY()

	/** 머지·신규 생성으로 갱신된 인스턴스들. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<TObjectPtr<UWxItemInstance>> TouchedInstances;

	/** 실제 추가된 수량. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 AmountAdded = 0;

	/** 슬롯 한도 초과 등으로 추가하지 못한 잔여 수량. 현재 구조상 항상 0. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Remainder = 0;
};

/**
 * 인벤토리 스택 변경 브로드캐스트.
 * NewCount 는 해당 ItemDef 의 소유 총합, Delta 는 이번 변경분(양수/음수).
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventoryStackChanged, const UWxItemDefinition* /*ItemDef*/, int32 /*NewCount*/, int32 /*Delta*/);

/**
 * 슬롯 단위 변경 브로드캐스트.
 * NewStackCount 는 해당 슬롯의 갱신 후 잔여 수량(제거 시 0), Delta 는 이번 변경분.
 * 동일 ItemDef 가 분할된 복수 슬롯에서 각자 갱신돼야 할 때 사용한다.
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventorySlotChanged, const UWxItemInstance* /*Instance*/, int32 /*NewStackCount*/, int32 /*Delta*/);

/**
 * 액터에 부착되어 아이템 인스턴스의 생성·소멸·레플리케이션을 관장하는 컴포넌트.
 *
 * 권한(서버)에서만 Add/Consume 이 호출되어야 하며, FastArray 로 클라이언트에 동기화된다.
 * 동일 ItemDef 요청은 MaxCounts 한도 내에서 기존 슬롯에 머지되고, 초과분은 새 슬롯으로 분할된다.
 * 재화(FWxItemFragment_Currency)는 MaxCounts 를 크게 잡아 단일 슬롯 누적으로 동작한다.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class WXINVENTORY_API UWxInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

	friend FWxInventoryList;

public:
	UWxInventoryManagerComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UActorComponent interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;
	//~ End UActorComponent interface

	/** 권한: ItemDef 를 머지 정책에 따라 추가한다. */
	FWxAddItemResult AddItem(const UWxItemDefinition* ItemDef, int32 Count = 1);

	/** 권한: 특정 인스턴스 슬롯을 통째로 제거한다. 재화 차감이 아닌 슬롯 파괴 용도. */
	void RemoveItem(UWxItemInstance* Instance);

	/** 권한: ItemDef 의 소유 수량을 Count 만큼 차감한다. 부족하면 false 반환하고 아무것도 차감하지 않는다(원자적). 0 이 된 슬롯은 제거한다. */
	bool ConsumeItemByDef(const UWxItemDefinition* ItemDef, int32 Count);

	/** 권한: Consumable Fragment를 가진 아이템을 1개 사용한다. 재고 부족/비 소비 아이템이면 false. 1개 차감 성공 후에만 GE를 소유 폰에 적용한다. */
	bool UseItemByDef(const UWxItemDefinition* ItemDef);

	/** 권한: Equipment Fragment를 가진 아이템을 소유 폰(IWxEquipmentInterface)에 장착 요청. 스택은 차감하지 않는다. ItemDef가 nullptr이면 장착 해제. */
	bool EquipItemByDef(const UWxItemDefinition* ItemDef);

	/** 권한: CurrencyTag 소유 총합에서 Count 만큼 차감한다. 원자적. */
	bool ConsumeItemByTag(FGameplayTag CurrencyTag, int32 Count);

	/** ItemDef 의 슬롯 합계. */
	int32 GetItemCountByDef(const UWxItemDefinition* ItemDef) const;

	/** CurrencyTag 와 일치하는 Currency Fragment 를 가진 아이템의 합계. */
	int32 GetItemCountByTag(FGameplayTag CurrencyTag) const;

	/** 특정 인스턴스가 속한 슬롯의 현재 StackCount. 인스턴스가 엔트리에 없으면 0. */
	int32 GetStackCountByInstance(const UWxItemInstance* Instance) const;

	TArray<UWxItemInstance*> GetAllItems() const;

	FWxOnInventoryStackChanged OnInventoryStackChanged;

	FWxOnInventorySlotChanged OnInventorySlotChanged;

private:
	/** 권한: Entries 를 돌며 머지/분할 처리. */
	void AddItemInternal(const UWxItemDefinition* ItemDef, int32 Count, FWxAddItemResult& OutResult);

	/** 권한: 단일 신규 엔트리 생성. */
	UWxItemInstance* CreateEntry(const UWxItemDefinition* ItemDef, int32 StackCount);

	/** ItemDef 합계 변경 브로드캐스트. 서버/클라이언트 공통 진입. */
	void BroadcastStackChanged(const UWxItemDefinition* ItemDef, int32 Delta);

	/** 슬롯 단위 변경 브로드캐스트. 서버/클라이언트 공통 진입. */
	void BroadcastSlotChanged(const UWxItemInstance* Instance, int32 NewStackCount, int32 Delta);

	UPROPERTY(Replicated)
	FWxInventoryList InventoryList;
};
