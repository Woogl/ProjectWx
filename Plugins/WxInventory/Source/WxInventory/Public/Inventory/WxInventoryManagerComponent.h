// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "WxInventoryManagerComponent.generated.h"

class UActorChannel;
class UWxItemDefinition;
class UWxItemInstance;
class UWxInventoryManagerComponent;
struct FWxInventoryList;

/**
 * 인벤토리 한 슬롯의 엔트리. 같은 ItemDef 라도 별개 슬롯으로 관리되며,
 * StackCount 머지 정책은 매니저 측에서 결정한다.
 */
USTRUCT(BlueprintType)
struct FWxInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FWxInventoryEntry();

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

	UWxItemInstance* AddEntry(const UWxItemDefinition* ItemDef, int32 StackCount);

	void RemoveEntry(UWxItemInstance* Instance);

	TArray<UWxItemInstance*> GetAllItems() const;

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
 * 액터에 부착되어 아이템 인스턴스의 생성·소멸·레플리케이션을 관장하는 컴포넌트.
 *
 * 권한(서버)에서만 Add/Remove 가 호출되어야 하며, FastArray 로 클라이언트에 동기화된다.
 * UWxItemInstance 는 SubObject 로 등록되어 함께 복제된다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXINVENTORY_API UWxInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxInventoryManagerComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UActorComponent interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	virtual void ReadyForReplication() override;
	//~ End UActorComponent interface

	UWxItemInstance* AddItem(const UWxItemDefinition* ItemDef, int32 StackCount = 1);

	void RemoveItem(UWxItemInstance* Instance);

	TArray<UWxItemInstance*> GetAllItems() const;

private:
	UPROPERTY(Replicated)
	FWxInventoryList InventoryList;
};
