// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxInventoryManagerComponent.h"

#include "Items/WxItemDefinition.h"
#include "Items/WxItemInstance.h"

#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

// ─────────────────────────────────────────────────────────────────────────────
// FWxInventoryEntry
// ─────────────────────────────────────────────────────────────────────────────

FWxInventoryEntry::FWxInventoryEntry()
	: Instance(nullptr)
	, StackCount(0)
	, LastObservedCount(INDEX_NONE)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// FWxInventoryList
// ─────────────────────────────────────────────────────────────────────────────

FWxInventoryList::FWxInventoryList()
	: OwnerComponent(nullptr)
{
}

FWxInventoryList::FWxInventoryList(UActorComponent* InOwnerComponent)
	: OwnerComponent(InOwnerComponent)
{
}

void FWxInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// 추후 메시지 브로드캐스트 지점.
}

void FWxInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FWxInventoryEntry& Entry = Entries[Index];
		Entry.LastObservedCount = Entry.StackCount;
	}
}

void FWxInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FWxInventoryEntry& Entry = Entries[Index];
		Entry.LastObservedCount = Entry.StackCount;
	}
}

bool FWxInventoryList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FWxInventoryEntry, FWxInventoryList>(Entries, DeltaParms, *this);
}

UWxItemInstance* FWxInventoryList::AddEntry(const UWxItemDefinition* ItemDef, int32 StackCount)
{
	if (!ItemDef || StackCount <= 0 || !OwnerComponent)
	{
		return nullptr;
	}

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor && OwningActor->HasAuthority());

	FWxInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UWxItemInstance>(OwningActor);
	NewEntry.Instance->SetItemDef(ItemDef);
	NewEntry.StackCount = StackCount;

	MarkItemDirty(NewEntry);
	return NewEntry.Instance;
}

void FWxInventoryList::RemoveEntry(UWxItemInstance* Instance)
{
	for (auto It = Entries.CreateIterator(); It; ++It)
	{
		if (It->Instance == Instance)
		{
			It.RemoveCurrent();
			MarkArrayDirty();
			break;
		}
	}
}

TArray<UWxItemInstance*> FWxInventoryList::GetAllItems() const
{
	TArray<UWxItemInstance*> Result;
	Result.Reserve(Entries.Num());
	for (const FWxInventoryEntry& Entry : Entries)
	{
		if (Entry.Instance)
		{
			Result.Add(Entry.Instance);
		}
	}
	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// UWxInventoryManagerComponent
// ─────────────────────────────────────────────────────────────────────────────

UWxInventoryManagerComponent::UWxInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)
{
	SetIsReplicatedByDefault(true);
}

void UWxInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}

bool UWxInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (const FWxInventoryEntry& Entry : InventoryList.Entries)
	{
		if (UWxItemInstance* Instance = Entry.Instance)
		{
			bWroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return bWroteSomething;
}

void UWxInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	if (IsUsingRegisteredSubObjectList())
	{
		for (const FWxInventoryEntry& Entry : InventoryList.Entries)
		{
			if (UWxItemInstance* Instance = Entry.Instance)
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

UWxItemInstance* UWxInventoryManagerComponent::AddItem(const UWxItemDefinition* ItemDef, int32 StackCount)
{
	UWxItemInstance* NewInstance = InventoryList.AddEntry(ItemDef, StackCount);

	if (NewInstance && IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		AddReplicatedSubObject(NewInstance);
	}

	return NewInstance;
}

void UWxInventoryManagerComponent::RemoveItem(UWxItemInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	if (IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(Instance);
	}

	InventoryList.RemoveEntry(Instance);
}

TArray<UWxItemInstance*> UWxInventoryManagerComponent::GetAllItems() const
{
	return InventoryList.GetAllItems();
}
