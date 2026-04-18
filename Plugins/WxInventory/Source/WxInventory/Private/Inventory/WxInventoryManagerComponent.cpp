// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxInventoryManagerComponent.h"

#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
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
	UWxInventoryManagerComponent* Manager = Cast<UWxInventoryManagerComponent>(OwnerComponent);
	if (!Manager)
	{
		return;
	}

	for (int32 Index : RemovedIndices)
	{
		const FWxInventoryEntry& Entry = Entries[Index];
		if (!Entry.Instance)
		{
			continue;
		}

		const UWxItemDefinition* ItemDef = Entry.Instance->GetItemDef();
		const int32 Delta = -Entry.LastObservedCount;
		Manager->BroadcastStackChanged(ItemDef, Delta);
	}
}

void FWxInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UWxInventoryManagerComponent* Manager = Cast<UWxInventoryManagerComponent>(OwnerComponent);

	for (int32 Index : AddedIndices)
	{
		FWxInventoryEntry& Entry = Entries[Index];
		Entry.LastObservedCount = Entry.StackCount;

		if (Manager && Entry.Instance)
		{
			Manager->BroadcastStackChanged(Entry.Instance->GetItemDef(), Entry.StackCount);
		}
	}
}

void FWxInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	UWxInventoryManagerComponent* Manager = Cast<UWxInventoryManagerComponent>(OwnerComponent);

	for (int32 Index : ChangedIndices)
	{
		FWxInventoryEntry& Entry = Entries[Index];
		const int32 Delta = Entry.StackCount - Entry.LastObservedCount;
		Entry.LastObservedCount = Entry.StackCount;

		if (Manager && Entry.Instance && Delta != 0)
		{
			Manager->BroadcastStackChanged(Entry.Instance->GetItemDef(), Delta);
		}
	}
}

bool FWxInventoryList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FWxInventoryEntry, FWxInventoryList>(Entries, DeltaParms, *this);
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

FWxAddItemResult UWxInventoryManagerComponent::AddItem(const UWxItemDefinition* ItemDef, int32 Count)
{
	FWxAddItemResult Result;
	if (!ItemDef || Count <= 0)
	{
		return Result;
	}

	AActor* OwningActor = GetOwner();
	check(OwningActor && OwningActor->HasAuthority());

	AddItemInternal(ItemDef, Count, Result);
	return Result;
}

void UWxInventoryManagerComponent::AddItemInternal(const UWxItemDefinition* ItemDef, int32 Count, FWxAddItemResult& OutResult)
{
	const int32 MaxPerSlot = FMath::Max(1, ItemDef->MaxCounts);
	int32 Remaining = Count;

	if (MaxPerSlot > 1)
	{
		for (FWxInventoryEntry& Entry : InventoryList.Entries)
		{
			if (Remaining <= 0)
			{
				break;
			}

			if (!Entry.Instance || Entry.Instance->GetItemDef() != ItemDef || Entry.StackCount >= MaxPerSlot)
			{
				continue;
			}

			const int32 Room = MaxPerSlot - Entry.StackCount;
			const int32 ToAdd = FMath::Min(Room, Remaining);
			Entry.StackCount += ToAdd;
			Remaining -= ToAdd;

			InventoryList.MarkItemDirty(Entry);
			OutResult.TouchedInstances.AddUnique(Entry.Instance);
			OutResult.AmountAdded += ToAdd;
			BroadcastStackChanged(ItemDef, ToAdd);
		}
	}

	while (Remaining > 0)
	{
		const int32 ToAdd = FMath::Min(MaxPerSlot, Remaining);
		UWxItemInstance* NewInstance = CreateEntry(ItemDef, ToAdd);
		if (!NewInstance)
		{
			break;
		}

		Remaining -= ToAdd;
		OutResult.TouchedInstances.Add(NewInstance);
		OutResult.AmountAdded += ToAdd;
		BroadcastStackChanged(ItemDef, ToAdd);
	}

	OutResult.Remainder = Remaining;
}

UWxItemInstance* UWxInventoryManagerComponent::CreateEntry(const UWxItemDefinition* ItemDef, int32 StackCount)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor)
	{
		return nullptr;
	}

	FWxInventoryEntry& NewEntry = InventoryList.Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UWxItemInstance>(OwningActor);
	NewEntry.Instance->SetItemDef(ItemDef);
	NewEntry.StackCount = StackCount;
	NewEntry.LastObservedCount = StackCount;

	InventoryList.MarkItemDirty(NewEntry);

	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		AddReplicatedSubObject(NewEntry.Instance);
	}

	return NewEntry.Instance;
}

void UWxInventoryManagerComponent::RemoveItem(UWxItemInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	for (auto It = InventoryList.Entries.CreateIterator(); It; ++It)
	{
		if (It->Instance != Instance)
		{
			continue;
		}

		const UWxItemDefinition* ItemDef = Instance->GetItemDef();
		const int32 Delta = -It->StackCount;

		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(Instance);
		}

		It.RemoveCurrent();
		InventoryList.MarkArrayDirty();
		BroadcastStackChanged(ItemDef, Delta);
		break;
	}
}

bool UWxInventoryManagerComponent::ConsumeItemByDef(const UWxItemDefinition* ItemDef, int32 Count)
{
	if (!ItemDef || Count <= 0)
	{
		return false;
	}

	check(GetOwner() && GetOwner()->HasAuthority());

	if (GetItemCountByDef(ItemDef) < Count)
	{
		return false;
	}

	int32 Remaining = Count;
	for (auto It = InventoryList.Entries.CreateIterator(); It && Remaining > 0; ++It)
	{
		if (!It->Instance || It->Instance->GetItemDef() != ItemDef)
		{
			continue;
		}

		const int32 ToTake = FMath::Min(It->StackCount, Remaining);
		It->StackCount -= ToTake;
		Remaining -= ToTake;

		if (It->StackCount <= 0)
		{
			if (IsUsingRegisteredSubObjectList())
			{
				RemoveReplicatedSubObject(It->Instance);
			}

			It.RemoveCurrent();
			InventoryList.MarkArrayDirty();
		}
		else
		{
			InventoryList.MarkItemDirty(*It);
		}
	}

	BroadcastStackChanged(ItemDef, -Count);
	return true;
}

bool UWxInventoryManagerComponent::ConsumeItemByTag(FGameplayTag CurrencyTag, int32 Count)
{
	if (!CurrencyTag.IsValid() || Count <= 0)
	{
		return false;
	}

	check(GetOwner() && GetOwner()->HasAuthority());

	if (GetItemCountByTag(CurrencyTag) < Count)
	{
		return false;
	}

	int32 Remaining = Count;
	for (auto It = InventoryList.Entries.CreateIterator(); It && Remaining > 0; ++It)
	{
		if (!It->Instance)
		{
			continue;
		}

		const UWxItemDefinition* ItemDef = It->Instance->GetItemDef();
		const FWxItemFragment_Currency* Currency = ItemDef ? ItemDef->FindFragment<FWxItemFragment_Currency>() : nullptr;
		if (!Currency || Currency->CurrencyTag != CurrencyTag)
		{
			continue;
		}

		const int32 ToTake = FMath::Min(It->StackCount, Remaining);
		It->StackCount -= ToTake;
		Remaining -= ToTake;
		BroadcastStackChanged(ItemDef, -ToTake);

		if (It->StackCount <= 0)
		{
			if (IsUsingRegisteredSubObjectList())
			{
				RemoveReplicatedSubObject(It->Instance);
			}

			It.RemoveCurrent();
			InventoryList.MarkArrayDirty();
		}
		else
		{
			InventoryList.MarkItemDirty(*It);
		}
	}

	return true;
}

int32 UWxInventoryManagerComponent::GetItemCountByDef(const UWxItemDefinition* ItemDef) const
{
	if (!ItemDef)
	{
		return 0;
	}

	int32 Total = 0;
	for (const FWxInventoryEntry& Entry : InventoryList.Entries)
	{
		if (Entry.Instance && Entry.Instance->GetItemDef() == ItemDef)
		{
			Total += Entry.StackCount;
		}
	}
	return Total;
}

int32 UWxInventoryManagerComponent::GetItemCountByTag(FGameplayTag CurrencyTag) const
{
	if (!CurrencyTag.IsValid())
	{
		return 0;
	}

	int32 Total = 0;
	for (const FWxInventoryEntry& Entry : InventoryList.Entries)
	{
		if (!Entry.Instance)
		{
			continue;
		}

		const FWxItemFragment_Currency* Currency = Entry.Instance->FindFragment<FWxItemFragment_Currency>();
		if (Currency && Currency->CurrencyTag == CurrencyTag)
		{
			Total += Entry.StackCount;
		}
	}
	return Total;
}

TArray<UWxItemInstance*> UWxInventoryManagerComponent::GetAllItems() const
{
	TArray<UWxItemInstance*> Result;
	Result.Reserve(InventoryList.Entries.Num());
	for (const FWxInventoryEntry& Entry : InventoryList.Entries)
	{
		if (Entry.Instance)
		{
			Result.Add(Entry.Instance);
		}
	}
	return Result;
}

void UWxInventoryManagerComponent::BroadcastStackChanged(const UWxItemDefinition* ItemDef, int32 Delta)
{
	if (!ItemDef || Delta == 0)
	{
		return;
	}

	const int32 NewCount = GetItemCountByDef(ItemDef);
	OnInventoryStackChanged.Broadcast(ItemDef, NewCount, Delta);
}
