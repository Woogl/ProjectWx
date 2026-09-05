// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxInventoryComponent.h"

#include "Inventory/WxEquipmentComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "WxGameplayTags.h"

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
	UWxInventoryComponent* Manager = Cast<UWxInventoryComponent>(OwnerComponent);
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

		// PreReplicatedRemove 는 엔트리가 실제 제거되기 "전"에 호출되므로, StackCount 를 먼저 0 으로 내려야 서버(제거 후 재계산) 경로와 같은 사후 총량이 발행된다.
		// 엔트리는 이 콜백 직후 실제 제거되므로 mutate 는 무해하다.
		Entry.StackCount = 0;
		Entry.LastObservedCount = 0;

		Manager->NotifySlotChangedFromList(Entry.Instance, 0, Delta);
		Manager->NotifyStackChangedFromList(Entry.Instance->GetItemDef(), Delta);
	}
}

void FWxInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UWxInventoryComponent* Manager = Cast<UWxInventoryComponent>(OwnerComponent);
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
			Manager->NotifySlotChangedFromList(Entry.Instance, Entry.StackCount, Entry.StackCount);
			Manager->NotifyStackChangedFromList(Entry.Instance->GetItemDef(), Entry.StackCount);
		}
	}
}

void FWxInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	UWxInventoryComponent* Manager = Cast<UWxInventoryComponent>(OwnerComponent);
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
			Manager->NotifySlotChangedFromList(Entry.Instance, Entry.StackCount, Delta);
			Manager->NotifyStackChangedFromList(Entry.Instance->GetItemDef(), Delta);
		}
	}
}

bool FWxInventoryList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FWxInventoryEntry, FWxInventoryList>(Entries, DeltaParms, *this);
}

void FWxInventoryList::PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters)
{
	// 미해결 인스턴스 참조가 나중에 매핑될 때도 호출된다. Delta가 0이어도 표시를 따라잡는다.
	if (UWxInventoryComponent* Manager = Cast<UWxInventoryComponent>(OwnerComponent))
	{
		Manager->NotifyContentsChangedFromReplication();
	}
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

int32 FWxInventoryList::AddToEntryStack(int32 EntryIndex, int32 Amount)
{
	FWxInventoryEntry& Entry = Entries[EntryIndex];
	Entry.StackCount += Amount;
	MarkItemDirty(Entry);
	return Entry.StackCount;
}

TArray<FWxInventoryChangeResult> FWxInventoryList::ConsumeByDefinition(const UWxItemDefinition* ItemDef, int32 NumToConsume)
{
	TArray<FWxInventoryChangeResult> Changes;

	int32 Remaining = NumToConsume;
	for (auto It = Entries.CreateIterator(); It && Remaining > 0; ++It)
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
			It.RemoveCurrent();
			MarkArrayDirty();
		}
		else
		{
			MarkItemDirty(*It);
		}

		Changes.Add({ SlotInstance, NewSlotCount, -ToTake });
	}

	return Changes;
}

const TArray<FWxInventoryEntry>& FWxInventoryList::GetEntries() const
{
	return Entries;
}

FWxOnInventoryReady UWxInventoryComponent::OnAnyInventoryReady;
FWxOnInventoryEnded UWxInventoryComponent::OnAnyInventoryEnded;

UWxInventoryComponent::UWxInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)
{
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void UWxInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	OnAnyInventoryReady.Broadcast(this);
}

void UWxInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// HasBegunPlay를 먼저 해제하여 종료 중인 자신에게 재연결하지 않도록 한다.
	Super::EndPlay(EndPlayReason);
	OnAnyInventoryEnded.Broadcast(this);
}
void UWxInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}

void UWxInventoryComponent::ReadyForReplication()
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

void UWxInventoryComponent::NotifyContentsChangedFromReplication()
{
	OnInventoryContentsChanged.Broadcast();
}

UWxItemInstance* UWxInventoryComponent::AddItemDefinition(const UWxItemDefinition* ItemDef, int32 StackCount)
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

	if (MaxStack > 1)
	{
		const TArray<FWxInventoryEntry>& Entries = InventoryList.GetEntries();
		for (int32 EntryIndex = 0; EntryIndex < Entries.Num() && Remaining > 0; ++EntryIndex)
		{
			UWxItemInstance* SlotInstance = Entries[EntryIndex].GetInstance();
			if (!SlotInstance || SlotInstance->GetItemDef() != ItemDef)
			{
				continue;
			}
			if (Entries[EntryIndex].GetStackCount() >= MaxStack)
			{
				continue;
			}

			const int32 ToAdd = FMath::Min(MaxStack - Entries[EntryIndex].GetStackCount(), Remaining);
			const int32 NewStackCount = InventoryList.AddToEntryStack(EntryIndex, ToAdd);
			Remaining -= ToAdd;

			NotifySlotChangedFromList(SlotInstance, NewStackCount, ToAdd);
			NotifyStackChangedFromList(ItemDef, ToAdd);

			if (!FirstAffected)
			{
				FirstAffected = SlotInstance;
			}
		}
	}

	while (Remaining > 0)
	{
		const int32 ChunkCount = FMath::Min(MaxStack, Remaining);
		UWxItemInstance* NewInstance = InventoryList.AddEntry(ItemDef, ChunkCount);
		RegisterReplicatedInstance(NewInstance);

		NotifySlotChangedFromList(NewInstance, ChunkCount, ChunkCount);
		NotifyStackChangedFromList(ItemDef, ChunkCount);

		// 충전형은 OnInstanceCreated 에서 초기 충전량이 set 만 되므로, 추가 시점에 원천이 직접 발행해야 먼저 초기화된 VM 에도 반영된다.
		const int32 InitialCharges = NewInstance->GetCurrentCharges();
		NotifyChargeChangedFromSource(NewInstance, InitialCharges, InitialCharges);

		Remaining -= ChunkCount;
		if (!FirstAffected)
		{
			FirstAffected = NewInstance;
		}
	}

	return FirstAffected;
}

void UWxInventoryComponent::GrantItems(const TArray<FWxItemRewardEntry>& Items)
{
	for (const FWxItemRewardEntry& Entry : Items)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		// 지급은 시작 시 1회 같은 산발적 호출이라 동기 로드해도 무방하다(픽업 비주얼 로드와 동일한 정책).
		if (const UWxItemDefinition* ItemDef = Entry.Item.LoadSynchronous())
		{
			AddItemDefinition(ItemDef, Entry.Quantity);
		}
	}
}

void UWxInventoryComponent::RemoveItemInstance(UWxItemInstance* ItemInstance)
{
	if (!ItemInstance)
	{
		return;
	}

	check(GetOwner() && GetOwner()->HasAuthority());

	int32 RemovedStackCount = 0;
	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		if (Entry.GetInstance() == ItemInstance)
		{
			RemovedStackCount = Entry.GetStackCount();
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

	NotifySlotChangedFromList(ItemInstance, 0, -RemovedStackCount);
	NotifyStackChangedFromList(RemovedDef, -RemovedStackCount);
}

bool UWxInventoryComponent::ConsumeItemsByDefinition(const UWxItemDefinition* ItemDef, int32 NumToConsume)
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

	const TArray<FWxInventoryChangeResult> Changes = InventoryList.ConsumeByDefinition(ItemDef, NumToConsume);
	for (const FWxInventoryChangeResult& Change : Changes)
	{
		// SubObject 등록 해제는 등록 리스트에만 영향이라 엔트리 제거 순서와 무관하다.
		if (Change.NewStackCount <= 0)
		{
			UnregisterReplicatedInstance(Change.Instance);
		}

		NotifySlotChangedFromList(Change.Instance, Change.NewStackCount, Change.Delta);
	}

	NotifyStackChangedFromList(ItemDef, -NumToConsume);
	return true;
}

UWxItemInstance* UWxInventoryComponent::FindFirstItemStackByDefinition(const UWxItemDefinition* ItemDef) const
{
	if (!ItemDef)
	{
		return nullptr;
	}

	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		UWxItemInstance* SlotInstance = Entry.GetInstance();
		if (SlotInstance && SlotInstance->GetItemDef() == ItemDef)
		{
			return SlotInstance;
		}
	}
	return nullptr;
}

int32 UWxInventoryComponent::GetTotalItemCountByDefinition(const UWxItemDefinition* ItemDef) const
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

int32 UWxInventoryComponent::GetStackCountByInstance(const UWxItemInstance* Instance) const
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

TArray<UWxItemInstance*> UWxInventoryComponent::GetAllItems() const
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

bool UWxInventoryComponent::RequestUseConsumable()
{
	const APlayerController* PC = GetOwner<APlayerController>();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC ? PC->GetPawn() : nullptr);
	if (!ASC)
	{
		return false;
	}

	return ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(WxGameplayTags::Ability_UseItem));
}

bool UWxInventoryComponent::CanUseItemByDef(const UWxItemDefinition* ItemDef) const
{
	return ItemDef
		&& ItemDef->FindFragmentByClass<UWxItemFragment_Usable>()
		&& FindUsableInstance(ItemDef) != nullptr;
}

bool UWxInventoryComponent::UseItemByDef(const UWxItemDefinition* ItemDef)
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

	UWxItemInstance* SourceInstance = FindUsableInstance(ItemDef);
	if (!SourceInstance)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = nullptr;
	FGameplayEffectSpecHandle Spec;
	if (Usable->Effect)
	{
		const APlayerController* PC = GetOwner<APlayerController>();
		TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC ? PC->GetPawn() : nullptr);
		if (!TargetASC)
		{
			return false;
		}

		// SourceObject 는 인스턴스 단위 데이터 추적이 가능하도록 ItemInstance 를 사용한다.
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		Context.AddSourceObject(SourceInstance);

		Spec = TargetASC->MakeOutgoingSpec(Usable->Effect, 1.f, Context);
		if (!Spec.IsValid())
		{
			return false;
		}
	}

	const UWxItemFragment_Charges* Charges = ItemDef->FindFragmentByClass<UWxItemFragment_Charges>();
	if (Charges)
	{
		const int32 OldCharges = SourceInstance->GetCurrentCharges();
		SourceInstance->SetCurrentCharges(OldCharges - 1);
		const int32 NewCharges = SourceInstance->GetCurrentCharges();
		NotifyChargeChangedFromSource(SourceInstance, NewCharges, NewCharges - OldCharges);
	}
	else if (!ConsumeItemsByDefinition(ItemDef, 1))
	{
		return false;
	}

	if (TargetASC && Spec.IsValid())
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}

	return true;
}

bool UWxInventoryComponent::RefillItemCharges(UWxItemInstance* Instance)
{
	if (!Instance)
	{
		return false;
	}

	check(GetOwner() && GetOwner()->HasAuthority());

	const int32 MaxCharges = Instance->GetMaxCharges();
	if (MaxCharges <= 0)
	{
		return false;
	}

	const int32 OldCharges = Instance->GetCurrentCharges();
	Instance->SetCurrentCharges(MaxCharges);
	const int32 NewCharges = Instance->GetCurrentCharges();
	NotifyChargeChangedFromSource(Instance, NewCharges, NewCharges - OldCharges);
	return true;
}

bool UWxInventoryComponent::EquipItemByDef(const UWxItemDefinition* ItemDef)
{
	check(GetOwner() && GetOwner()->HasAuthority());

	if (ItemDef)
	{
		if (!ItemDef->FindFragmentByClass<UWxItemFragment_Equippable>())
		{
			return false;
		}
		if (!FindFirstItemStackByDefinition(ItemDef))
		{
			return false;
		}
	}

	const APlayerController* PC = GetOwner<APlayerController>();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UWxEquipmentComponent* Equipment = Pawn ? Pawn->FindComponentByClass<UWxEquipmentComponent>() : nullptr;
	if (!Equipment)
	{
		return false;
	}

	Equipment->EquipItem(ItemDef);
	return true;
}

void UWxInventoryComponent::NotifyStackChangedFromList(const UWxItemDefinition* ItemDef, int32 Delta)
{
	if (!ItemDef || Delta == 0)
	{
		return;
	}

	const int32 NewCount = GetTotalItemCountByDefinition(ItemDef);
	OnInventoryStackChanged.Broadcast(ItemDef, NewCount, Delta);
}

void UWxInventoryComponent::NotifySlotChangedFromList(UWxItemInstance* Instance, int32 NewStackCount, int32 Delta)
{
	if (!Instance || Delta == 0)
	{
		return;
	}

	OnInventorySlotChanged.Broadcast(Instance, NewStackCount, Delta);
}

void UWxInventoryComponent::NotifyChargeChangedFromSource(UWxItemInstance* Instance, int32 NewCharges, int32 Delta)
{
	if (!Instance || Delta == 0)
	{
		return;
	}

	OnInventoryChargeChanged.Broadcast(Instance, NewCharges, Delta);
}

UWxItemInstance* UWxInventoryComponent::FindUsableInstance(const UWxItemDefinition* ItemDef) const
{
	if (!ItemDef)
	{
		return nullptr;
	}

	const UWxItemFragment_Charges* Charges = ItemDef->FindFragmentByClass<UWxItemFragment_Charges>();

	for (const FWxInventoryEntry& Entry : InventoryList.GetEntries())
	{
		UWxItemInstance* SlotInstance = Entry.GetInstance();
		if (!SlotInstance || SlotInstance->GetItemDef() != ItemDef)
		{
			continue;
		}
		if (Charges && SlotInstance->GetCurrentCharges() <= 0)
		{
			continue;
		}

		return SlotInstance;
	}

	return nullptr;
}

void UWxInventoryComponent::RegisterReplicatedInstance(UWxItemInstance* Instance)
{
	if (Instance && IsReadyForReplication())
	{
		AddReplicatedSubObject(Instance);
	}
}

void UWxInventoryComponent::UnregisterReplicatedInstance(UWxItemInstance* Instance)
{
	if (Instance && IsReadyForReplication())
	{
		RemoveReplicatedSubObject(Instance);
	}
}
