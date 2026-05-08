// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "WxInventoryManagerComponent.generated.h"

class UWxItemDefinition;
class UWxItemInstance;
class UWxInventoryManagerComponent;
struct FWxInventoryList;

/**
 * 인벤토리 한 슬롯의 엔트리.
 *
 * AddItemDefinition 은 ItemDef 의 Stackable Fragment(MaxStack) 를 기준으로 기존 엔트리에 머지하고,
 * 한도를 넘는 잔여분은 새 엔트리로 분할한다. Stackable Fragment 가 없으면 항상 1슬롯 = 1개로 강제된다.
 */
USTRUCT(BlueprintType)
struct FWxInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FWxInventoryEntry();

	UWxItemInstance* GetInstance() const;
	int32 GetStackCount() const;

private:
	friend FWxInventoryList;
	friend UWxInventoryManagerComponent;

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

	/** 권한: 새 인스턴스를 생성해 신규 엔트리로 추가. Fragment 의 OnInstanceCreated 가 호출된다. */
	UWxItemInstance* AddEntry(const UWxItemDefinition* ItemDef, int32 StackCount);

	/** 권한: 인스턴스에 해당하는 엔트리를 통째로 제거. */
	void RemoveEntry(UWxItemInstance* Instance);

	const TArray<FWxInventoryEntry>& GetEntries() const;

private:
	friend UWxInventoryManagerComponent;

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
 * 인벤토리 정의 단위 합계 변경 브로드캐스트.
 * NewCount 는 해당 ItemDef 의 소유 총합, Delta 는 이번 변경분(양수/음수).
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventoryStackChanged, const UWxItemDefinition* /*ItemDef*/, int32 /*NewCount*/, int32 /*Delta*/);

/**
 * 슬롯(인스턴스) 단위 변경 브로드캐스트.
 * NewStackCount 는 해당 슬롯의 갱신 후 잔여 수량(제거 시 0), Delta 는 이번 변경분.
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxOnInventorySlotChanged, UWxItemInstance* /*Instance*/, int32 /*NewStackCount*/, int32 /*Delta*/);

/**
 * 액터에 부착되어 아이템 인스턴스의 생성·소멸·레플리케이션을 관장하는 컴포넌트.
 *
 * AddItemDefinition 은 ItemDef 의 Stackable Fragment 한도(MaxStack) 까지 기존 엔트리에 머지하고,
 * 초과분은 새 엔트리들로 분할한다. Stackable Fragment 가 없는 ItemDef 는 항상 1슬롯 = 1개로 추가된다.
 * 정의 합계는 GetTotalItemCountByDefinition 로 조회하고, 차감은 ConsumeItemsByDefinition 로 수행한다.
 *
 * 권한(서버)에서만 Add/Consume 이 호출되어야 하며, FastArray 로 클라이언트에 동기화된다.
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
	virtual void ReadyForReplication() override;
	//~ End UActorComponent interface

	/**
	 * 임의 액터에서 인벤토리 매니저를 찾아 반환한다.
	 * 인벤토리는 PlayerController 에 부착되므로, 액터가 Pawn 이면 소유 컨트롤러를 거쳐 조회한다.
	 * PlayerController 가 아닌 액터(또는 컨트롤러 미할당 폰) 는 nullptr.
	 */
	static UWxInventoryManagerComponent* FindInventory(const AActor* Actor);

	/**
	 * 권한: ItemDef 를 StackCount 만큼 추가한다.
	 * Stackable Fragment 가 있으면 기존 엔트리에 MaxStack 한도까지 머지하고, 초과분은 새 엔트리들로 분할한다.
	 * Stackable Fragment 가 없으면 StackCount 만큼의 신규 엔트리(각 1개) 가 생성된다.
	 * 반환값은 첫 영향받은 인스턴스(머지된 기존 엔트리 또는 새로 만든 첫 엔트리). 실패 시 nullptr.
	 */
	UWxItemInstance* AddItemDefinition(const UWxItemDefinition* ItemDef, int32 StackCount = 1);

	/** 권한: 특정 인스턴스 슬롯을 통째로 제거한다. */
	void RemoveItemInstance(UWxItemInstance* ItemInstance);

	/**
	 * 권한: ItemDef 의 소유 수량을 NumToConsume 만큼 차감한다. 부족하면 false 반환하고 아무것도 차감하지 않는다(원자적).
	 * 0 이 된 슬롯은 제거한다. 같은 ItemDef 가 복수 엔트리로 분산돼 있어도 합산 차감이 가능하다.
	 */
	bool ConsumeItemsByDefinition(const UWxItemDefinition* ItemDef, int32 NumToConsume);

	/** ItemDef 의 첫 번째 인스턴스 반환. 없으면 nullptr. */
	UWxItemInstance* FindFirstItemStackByDefinition(const UWxItemDefinition* ItemDef) const;

	/** ItemDef 의 모든 엔트리 StackCount 합계. */
	int32 GetTotalItemCountByDefinition(const UWxItemDefinition* ItemDef) const;

	/** 특정 인스턴스가 속한 슬롯의 현재 StackCount. 인스턴스가 엔트리에 없으면 0. */
	int32 GetStackCountByInstance(const UWxItemInstance* Instance) const;

	TArray<UWxItemInstance*> GetAllItems() const;

	/** 권한: Consumable Fragment 를 가진 아이템을 1개 사용한다. 재고 부족/비 소비 아이템이면 false. 1개 차감 성공 후에만 GE를 소유 폰에 적용한다. */
	bool UseItemByDef(const UWxItemDefinition* ItemDef);

	/** 권한: Equipment Fragment 를 가진 아이템을 소유 폰(IWxEquipmentInterface)에 장착 요청. 스택은 차감하지 않는다. ItemDef 가 nullptr 이면 장착 해제. */
	bool EquipItemByDef(const UWxItemDefinition* ItemDef);

	FWxOnInventoryStackChanged OnInventoryStackChanged;

	FWxOnInventorySlotChanged OnInventorySlotChanged;

private:
	/** 신규 인스턴스를 SubObject 시스템에 등록한다. */
	void RegisterReplicatedInstance(UWxItemInstance* Instance);

	/** 인스턴스를 SubObject 시스템에서 해제한다. */
	void UnregisterReplicatedInstance(UWxItemInstance* Instance);

	/** ItemDef 합계 변경 브로드캐스트. 서버/클라이언트 공통 진입. */
	void BroadcastStackChanged(const UWxItemDefinition* ItemDef, int32 Delta);

	/** 슬롯 단위 변경 브로드캐스트. 서버/클라이언트 공통 진입. */
	void BroadcastSlotChanged(UWxItemInstance* Instance, int32 NewStackCount, int32 Delta);

	UPROPERTY(Replicated)
	FWxInventoryList InventoryList;
};
