// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxInventoryManagerComponent.h"

#include "Inventory/WxEquipmentInterface.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/PlayerState.h"
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

UWxItemInstance* FWxInventoryEntry::GetInstance() const
{
	return Instance;
}

int32 FWxInventoryEntry::GetStackCount() const
{
	return StackCount;
}

void FWxInventoryEntry::AddStack(int32 Delta)
{
	StackCount += Delta;
}

void FWxInventoryEntry::Initialize(UWxItemInstance* InInstance, int32 InStackCount)
{
	Instance = InInstance;
	StackCount = InStackCount;
	LastObservedCount = InStackCount;
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
		Manager->BroadcastSlotChanged(Entry.Instance, 0, Delta);
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
			Manager->BroadcastSlotChanged(Entry.Instance, Entry.StackCount, Entry.StackCount);
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
			Manager->BroadcastSlotChanged(Entry.Instance, Entry.StackCount, Delta);
			Manager->BroadcastStackChanged(Entry.Instance->GetItemDef(), Delta);
		}
	}
}

bool FWxInventoryList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FWxInventoryEntry, FWxInventoryList>(Entries, DeltaParms, *this);
}

const TArray<FWxInventoryEntry>& FWxInventoryList::GetEntries() const
{
	return Entries;
}

TArray<FWxInventoryEntry>& FWxInventoryList::GetEntriesMutable()
{
	return Entries;
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

	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		if (UWxItemInstance* Instance = Entry.GetInstance())
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
		for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
		{
			if (UWxItemInstance* Instance = Entry.GetInstance())
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
		for (FWxInventoryEntry& Entry : InventoryList.GetEntriesMutable())
		{
			if (Remaining <= 0)
			{
				break;
			}

			UWxItemInstance* SlotInstance = Entry.GetInstance();
			if (!SlotInstance || SlotInstance->GetItemDef() != ItemDef || Entry.GetStackCount() >= MaxPerSlot)
			{
				continue;
			}

			const int32 Room = MaxPerSlot - Entry.GetStackCount();
			const int32 ToAdd = FMath::Min(Room, Remaining);
			Entry.AddStack(ToAdd);
			Remaining -= ToAdd;

			InventoryList.MarkItemDirty(Entry);
			OutResult.TouchedInstances.AddUnique(SlotInstance);
			OutResult.AmountAdded += ToAdd;
			BroadcastSlotChanged(SlotInstance, Entry.GetStackCount(), ToAdd);
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
		BroadcastSlotChanged(NewInstance, ToAdd, ToAdd);
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

	UWxItemInstance* NewInstance = NewObject<UWxItemInstance>(OwningActor);
	NewInstance->SetItemDef(ItemDef);

	FWxInventoryEntry& NewEntry = InventoryList.GetEntriesMutable().AddDefaulted_GetRef();
	NewEntry.Initialize(NewInstance, StackCount);

	InventoryList.MarkItemDirty(NewEntry);

	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
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

	for (auto It = InventoryList.GetEntriesMutable().CreateIterator(); It; ++It)
	{
		if (It->GetInstance() != Instance)
		{
			continue;
		}

		const UWxItemDefinition* ItemDef = Instance->GetItemDef();
		const int32 Delta = -It->GetStackCount();

		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(Instance);
		}

		It.RemoveCurrent();
		InventoryList.MarkArrayDirty();
		BroadcastSlotChanged(Instance, 0, Delta);
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
	for (auto It = InventoryList.GetEntriesMutable().CreateIterator(); It && Remaining > 0; ++It)
	{
		UWxItemInstance* SlotInstance = It->GetInstance();
		if (!SlotInstance || SlotInstance->GetItemDef() != ItemDef)
		{
			continue;
		}

		const int32 ToTake = FMath::Min(It->GetStackCount(), Remaining);
		It->AddStack(-ToTake);
		Remaining -= ToTake;

		const int32 NewSlotCount = It->GetStackCount();

		if (NewSlotCount <= 0)
		{
			if (IsUsingRegisteredSubObjectList())
			{
				RemoveReplicatedSubObject(SlotInstance);
			}

			It.RemoveCurrent();
			InventoryList.MarkArrayDirty();
		}
		else
		{
			InventoryList.MarkItemDirty(*It);
		}

		BroadcastSlotChanged(SlotInstance, NewSlotCount, -ToTake);
	}

	BroadcastStackChanged(ItemDef, -Count);
	return true;
}

bool UWxInventoryManagerComponent::UseItemByDef(const UWxItemDefinition* ItemDef)
{
	if (!ItemDef)
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();
	check(OwnerActor && OwnerActor->HasAuthority());

	const FWxItemFragment_Consumable* Consumable = ItemDef->FindFragment<FWxItemFragment_Consumable>();
	if (!Consumable)
	{
		return false;
	}

	// 재고 부족이면 적용도 차감도 발생하지 않아야 하므로, 차감을 먼저 시도한다.
	if (!ConsumeItemByDef(ItemDef, 1))
	{
		return false;
	}

	if (!Consumable->Effect)
	{
		return true;
	}

	// PlayerState에 부착된 경우 실제 ASC는 소유 폰이 가진다. 범용 액터 소유도 지원하기 위해 PlayerState → Pawn → Owner 순으로 조회.
	UAbilitySystemComponent* TargetASC = nullptr;
	if (const APlayerState* PS = Cast<APlayerState>(OwnerActor))
	{
		TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS->GetPawn());
	}
	if (!TargetASC)
	{
		TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	}

	if (!TargetASC)
	{
		return true;
	}

	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	Context.AddSourceObject(ItemDef);

	const FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(Consumable->Effect, 1.f, Context);
	if (Spec.IsValid())
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}

	return true;
}

bool UWxInventoryManagerComponent::EquipItemByDef(const UWxItemDefinition* ItemDef)
{
	AActor* OwnerActor = GetOwner();
	check(OwnerActor && OwnerActor->HasAuthority());

	// 장착 해제(nullptr)는 그대로 전달한다. 장착일 경우에만 Fragment 유효성 검증.
	if (ItemDef && !ItemDef->FindFragment<FWxItemFragment_Equipment>())
	{
		return false;
	}

	// PlayerState에 부착된 경우 실제 장착 대상은 소유 폰.
	AActor* TargetActor = OwnerActor;
	if (const APlayerState* PS = Cast<APlayerState>(OwnerActor))
	{
		TargetActor = PS->GetPawn();
	}

	IWxEquipmentInterface* Interface = Cast<IWxEquipmentInterface>(TargetActor);
	if (!Interface)
	{
		return false;
	}

	Interface->EquipItem(ItemDef);
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
	for (auto It = InventoryList.GetEntriesMutable().CreateIterator(); It && Remaining > 0; ++It)
	{
		UWxItemInstance* SlotInstance = It->GetInstance();
		if (!SlotInstance)
		{
			continue;
		}

		const UWxItemDefinition* ItemDef = SlotInstance->GetItemDef();
		const FWxItemFragment_Currency* Currency = ItemDef ? ItemDef->FindFragment<FWxItemFragment_Currency>() : nullptr;
		if (!Currency || Currency->CurrencyTag != CurrencyTag)
		{
			continue;
		}

		const int32 ToTake = FMath::Min(It->GetStackCount(), Remaining);
		It->AddStack(-ToTake);
		Remaining -= ToTake;

		const int32 NewSlotCount = It->GetStackCount();

		if (NewSlotCount <= 0)
		{
			if (IsUsingRegisteredSubObjectList())
			{
				RemoveReplicatedSubObject(SlotInstance);
			}

			It.RemoveCurrent();
			InventoryList.MarkArrayDirty();
		}
		else
		{
			InventoryList.MarkItemDirty(*It);
		}

		BroadcastSlotChanged(SlotInstance, NewSlotCount, -ToTake);
		BroadcastStackChanged(ItemDef, -ToTake);
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
	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		const UWxItemInstance* SlotInstance = Entry.GetInstance();
		if (SlotInstance && SlotInstance->GetItemDef() == ItemDef)
		{
			Total += Entry.GetStackCount();
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
	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		const UWxItemInstance* SlotInstance = Entry.GetInstance();
		if (!SlotInstance)
		{
			continue;
		}

		const FWxItemFragment_Currency* Currency = SlotInstance->FindFragment<FWxItemFragment_Currency>();
		if (Currency && Currency->CurrencyTag == CurrencyTag)
		{
			Total += Entry.GetStackCount();
		}
	}
	return Total;
}

int32 UWxInventoryManagerComponent::GetStackCountByInstance(const UWxItemInstance* Instance) const
{
	if (!Instance)
	{
		return 0;
	}

	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		if (Entry.GetInstance() == Instance)
		{
			return Entry.GetStackCount();
		}
	}
	return 0;
}

TArray<UWxItemInstance*> UWxInventoryManagerComponent::GetAllItems() const
{
	const TArray<FWxInventoryEntry>& Entries = InventoryList.GetEntries();
	TArray<UWxItemInstance*> Result;
	Result.Reserve(Entries.Num());
	for (const FWxInventoryEntry& Entry : Entries)
	{
		if (UWxItemInstance* SlotInstance = Entry.GetInstance())
		{
			Result.Add(SlotInstance);
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

void UWxInventoryManagerComponent::BroadcastSlotChanged(const UWxItemInstance* Instance, int32 NewStackCount, int32 Delta)
{
	if (!Instance || Delta == 0)
	{
		return;
	}

	OnInventorySlotChanged.Broadcast(Instance, NewStackCount, Delta);
}
