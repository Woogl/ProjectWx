// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxInventoryManagerComponent.h"

#include "Inventory/WxEquipmentInterface.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
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
		FWxInventoryEntry& Entry = Entries[Index];
		if (!Entry.Instance)
		{
			continue;
		}

		const int32 Delta = -Entry.LastObservedCount;
		Manager->BroadcastSlotChanged(Entry.Instance, 0, Delta);
		Manager->BroadcastStackChanged(Entry.Instance->GetItemDef(), Delta);
		Entry.LastObservedCount = 0;
	}
}

void FWxInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UWxInventoryManagerComponent* Manager = Cast<UWxInventoryManagerComponent>(OwnerComponent);
	if (!Manager)
	{
		return;
	}

	for (int32 Index : AddedIndices)
	{
		FWxInventoryEntry& Entry = Entries[Index];
		Entry.LastObservedCount = Entry.StackCount;

		if (Entry.Instance)
		{
			Manager->BroadcastSlotChanged(Entry.Instance, Entry.StackCount, Entry.StackCount);
			Manager->BroadcastStackChanged(Entry.Instance->GetItemDef(), Entry.StackCount);
		}
	}
}

void FWxInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	UWxInventoryManagerComponent* Manager = Cast<UWxInventoryManagerComponent>(OwnerComponent);
	if (!Manager)
	{
		return;
	}

	for (int32 Index : ChangedIndices)
	{
		FWxInventoryEntry& Entry = Entries[Index];
		const int32 Delta = Entry.StackCount - Entry.LastObservedCount;
		Entry.LastObservedCount = Entry.StackCount;

		if (Entry.Instance && Delta != 0)
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

UWxItemInstance* FWxInventoryList::AddEntry(const UWxItemDefinition* ItemDef, int32 StackCount)
{
	check(ItemDef);
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor && OwningActor->HasAuthority());

	FWxInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UWxItemInstance>(OwningActor);
	NewEntry.Instance->SetItemDef(ItemDef);
	NewEntry.StackCount = StackCount;
	NewEntry.LastObservedCount = StackCount;

	for (UWxItemFragment* Fragment : ItemDef->Fragments)
	{
		if (Fragment)
		{
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}

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
			return;
		}
	}
}

const TArray<FWxInventoryEntry>& FWxInventoryList::GetEntries() const
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
	bReplicateUsingRegisteredSubObjectList = true;
}

void UWxInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}

void UWxInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		if (UWxItemInstance* Instance = Entry.GetInstance())
		{
			AddReplicatedSubObject(Instance);
		}
	}
}

UWxItemInstance* UWxInventoryManagerComponent::AddItemDefinition(const UWxItemDefinition* ItemDef, int32 StackCount)
{
	if (!ItemDef || StackCount <= 0)
	{
		return nullptr;
	}

	check(GetOwner() && GetOwner()->HasAuthority());

	const UWxItemFragment_Stackable* Stackable = ItemDef->FindFragmentByClass<UWxItemFragment_Stackable>();
	const int32 MaxStack = Stackable ? Stackable->MaxStack : 1;

	int32 Remaining = StackCount;
	UWxItemInstance* FirstAffected = nullptr;

	// 1) 기존 엔트리에 머지 (MaxStack > 1 인 스택 가능 아이템에만 해당).
	if (MaxStack > 1)
	{
		for (FWxInventoryEntry& Entry : InventoryList.Entries)
		{
			if (Remaining <= 0)
			{
				break;
			}
			if (!Entry.Instance || Entry.Instance->GetItemDef() != ItemDef)
			{
				continue;
			}
			if (Entry.StackCount >= MaxStack)
			{
				continue;
			}

			const int32 Capacity = MaxStack - Entry.StackCount;
			const int32 ToAdd = FMath::Min(Capacity, Remaining);
			Entry.StackCount += ToAdd;
			Remaining -= ToAdd;

			InventoryList.MarkItemDirty(Entry);
			BroadcastSlotChanged(Entry.Instance, Entry.StackCount, ToAdd);
			BroadcastStackChanged(ItemDef, ToAdd);

			if (!FirstAffected)
			{
				FirstAffected = Entry.Instance;
			}
		}
	}

	// 2) 잔여분은 새 엔트리로 분할 생성. MaxStack 단위로 chunk 화.
	while (Remaining > 0)
	{
		const int32 ChunkCount = FMath::Min(MaxStack, Remaining);
		UWxItemInstance* NewInstance = InventoryList.AddEntry(ItemDef, ChunkCount);
		RegisterReplicatedInstance(NewInstance);

		BroadcastSlotChanged(NewInstance, ChunkCount, ChunkCount);
		BroadcastStackChanged(ItemDef, ChunkCount);

		Remaining -= ChunkCount;
		if (!FirstAffected)
		{
			FirstAffected = NewInstance;
		}
	}

	return FirstAffected;
}

void UWxInventoryManagerComponent::RemoveItemInstance(UWxItemInstance* ItemInstance)
{
	if (!ItemInstance)
	{
		return;
	}

	check(GetOwner() && GetOwner()->HasAuthority());

	int32 RemovedStackCount = 0;
	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		if (Entry.Instance == ItemInstance)
		{
			RemovedStackCount = Entry.StackCount;
			break;
		}
	}

	if (RemovedStackCount <= 0)
	{
		return;
	}

	const UWxItemDefinition* RemovedDef = ItemInstance->GetItemDef();

	UnregisterReplicatedInstance(ItemInstance);

	InventoryList.RemoveEntry(ItemInstance);

	BroadcastSlotChanged(ItemInstance, 0, -RemovedStackCount);
	BroadcastStackChanged(RemovedDef, -RemovedStackCount);
}

bool UWxInventoryManagerComponent::ConsumeItemsByDefinition(const UWxItemDefinition* ItemDef, int32 NumToConsume)
{
	if (!ItemDef || NumToConsume <= 0)
	{
		return false;
	}

	check(GetOwner() && GetOwner()->HasAuthority());

	if (GetTotalItemCountByDefinition(ItemDef) < NumToConsume)
	{
		return false;
	}

	int32 Remaining = NumToConsume;
	for (auto It = InventoryList.Entries.CreateIterator(); It && Remaining > 0; ++It)
	{
		UWxItemInstance* SlotInstance = It->Instance;
		if (!SlotInstance || SlotInstance->GetItemDef() != ItemDef)
		{
			continue;
		}

		const int32 ToTake = FMath::Min(It->StackCount, Remaining);
		It->StackCount -= ToTake;
		Remaining -= ToTake;

		const int32 NewSlotCount = It->StackCount;

		if (NewSlotCount <= 0)
		{
			UnregisterReplicatedInstance(SlotInstance);

			It.RemoveCurrent();
			InventoryList.MarkArrayDirty();
		}
		else
		{
			InventoryList.MarkItemDirty(*It);
		}

		BroadcastSlotChanged(SlotInstance, NewSlotCount, -ToTake);
	}

	BroadcastStackChanged(ItemDef, -NumToConsume);
	return true;
}

UWxItemInstance* UWxInventoryManagerComponent::FindFirstItemStackByDefinition(const UWxItemDefinition* ItemDef) const
{
	if (!ItemDef)
	{
		return nullptr;
	}

	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		UWxItemInstance* SlotInstance = Entry.Instance;
		if (SlotInstance && SlotInstance->GetItemDef() == ItemDef)
		{
			return SlotInstance;
		}
	}
	return nullptr;
}

int32 UWxInventoryManagerComponent::GetTotalItemCountByDefinition(const UWxItemDefinition* ItemDef) const
{
	if (!ItemDef)
	{
		return 0;
	}

	int32 Total = 0;
	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		const UWxItemInstance* SlotInstance = Entry.Instance;
		if (SlotInstance && SlotInstance->GetItemDef() == ItemDef)
		{
			Total += Entry.StackCount;
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
		if (Entry.Instance == Instance)
		{
			return Entry.StackCount;
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
		if (UWxItemInstance* SlotInstance = Entry.Instance)
		{
			Result.Add(SlotInstance);
		}
	}
	return Result;
}

bool UWxInventoryManagerComponent::UseItemByDef(const UWxItemDefinition* ItemDef)
{
	if (!ItemDef)
	{
		return false;
	}

	check(GetOwner() && GetOwner()->HasAuthority());

	const UWxItemFragment_Usable* Usable = ItemDef->FindFragmentByClass<UWxItemFragment_Usable>();
	if (!Usable)
	{
		return false;
	}

	// 재고 사전 검증 — 차감하지 않고 가용 여부만 확인.
	if (GetTotalItemCountByDefinition(ItemDef) <= 0)
	{
		return false;
	}

	// GE가 지정된 경우, ASC와 Spec 유효성을 차감 전에 검증한다. 검증 실패 시 차감하지 않고 false 반환.
	UAbilitySystemComponent* TargetASC = nullptr;
	FGameplayEffectSpecHandle Spec;
	if (Usable->Effect)
	{
		TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ResolveTargetActor());
		if (!TargetASC)
		{
			return false;
		}

		// SourceObject 는 인스턴스 단위 데이터 추적이 가능하도록 ItemInstance 를 사용한다.
		UWxItemInstance* SourceInstance = FindFirstItemStackByDefinition(ItemDef);
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		Context.AddSourceObject(SourceInstance);

		Spec = TargetASC->MakeOutgoingSpec(Usable->Effect, 1.f, Context);
		if (!Spec.IsValid())
		{
			return false;
		}
	}

	if (!ConsumeItemsByDefinition(ItemDef, 1))
	{
		return false;
	}

	if (TargetASC && Spec.IsValid())
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}

	return true;
}

bool UWxInventoryManagerComponent::EquipItemByDef(const UWxItemDefinition* ItemDef)
{
	check(GetOwner() && GetOwner()->HasAuthority());

	// 장착 해제(nullptr)는 그대로 전달한다. 장착일 경우에만 Fragment 유효성 검증.
	if (ItemDef && !ItemDef->FindFragmentByClass<UWxItemFragment_Equippable>())
	{
		return false;
	}

	IWxEquipmentInterface* Interface = Cast<IWxEquipmentInterface>(ResolveTargetActor());
	if (!Interface)
	{
		return false;
	}

	Interface->EquipItem(ItemDef);
	return true;
}

AActor* UWxInventoryManagerComponent::ResolveTargetActor() const
{
	AActor* OwnerActor = GetOwner();
	if (const APlayerController* PC = Cast<APlayerController>(OwnerActor))
	{
		// PC 부착 시 폰이 정답. 폰이 없으면 ASC 도 없으므로 nullptr 로 명시 — 호출자가 가드.
		return PC->GetPawn();
	}
	return OwnerActor;
}

void UWxInventoryManagerComponent::RegisterReplicatedInstance(UWxItemInstance* Instance)
{
	if (Instance && IsReadyForReplication())
	{
		AddReplicatedSubObject(Instance);
	}
}

void UWxInventoryManagerComponent::UnregisterReplicatedInstance(UWxItemInstance* Instance)
{
	if (Instance && IsReadyForReplication())
	{
		RemoveReplicatedSubObject(Instance);
	}
}

void UWxInventoryManagerComponent::BroadcastStackChanged(const UWxItemDefinition* ItemDef, int32 Delta)
{
	if (!ItemDef || Delta == 0)
	{
		return;
	}

	const int32 NewCount = GetTotalItemCountByDefinition(ItemDef);
	OnInventoryStackChanged.Broadcast(ItemDef, NewCount, Delta);
}

void UWxInventoryManagerComponent::BroadcastSlotChanged(UWxItemInstance* Instance, int32 NewStackCount, int32 Delta)
{
	if (!Instance || Delta == 0)
	{
		return;
	}

	OnInventorySlotChanged.Broadcast(Instance, NewStackCount, Delta);
}
