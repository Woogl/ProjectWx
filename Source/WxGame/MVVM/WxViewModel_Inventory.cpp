// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Inventory.h"

#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemInstance.h"
#include "MVVM/WxViewModel_Item.h"

void UWxViewModel_Inventory::Initialize(UWxInventoryManagerComponent* InInventory)
{
	if (!InInventory)
	{
		return;
	}

	Deinitialize();

	CachedInventory = InInventory;
	StackChangedHandle = InInventory->OnInventoryStackChanged.AddUObject(this, &UWxViewModel_Inventory::HandleStackChanged);

	RefreshAllItems();

	SetInitialized(true);
}

void UWxViewModel_Inventory::Deinitialize()
{
	if (UWxInventoryManagerComponent* Inventory = CachedInventory.Get())
	{
		Inventory->OnInventoryStackChanged.Remove(StackChangedHandle);
	}
	StackChangedHandle.Reset();
	CachedInventory.Reset();

	LastChangedItemDef = nullptr;
	LastChangedAmount = 0;
	LastChangedDelta = 0;

	for (UWxViewModel_Item* ChildVM : AllItems)
	{
		if (ChildVM)
		{
			ChildVM->Deinitialize();
		}
	}
	AllItems.Reset();
	CategorizedItems.Reset();

	SetInitialized(false);

	Super::Deinitialize();
}

int32 UWxViewModel_Inventory::GetCurrencyAmount(const UWxItemDefinition* ItemDef) const
{
	const UWxInventoryManagerComponent* Inventory = CachedInventory.Get();
	return Inventory ? Inventory->GetTotalItemCountByDefinition(ItemDef) : 0;
}

void UWxViewModel_Inventory::HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta)
{
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedItemDef, ItemDef);
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedAmount, NewCount);
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedDelta, Delta);

	RefreshAllItems();
}

void UWxViewModel_Inventory::RefreshAllItems()
{
	UWxInventoryManagerComponent* Inventory = CachedInventory.Get();
	TArray<TObjectPtr<UWxViewModel_Item>> NewItems;
	TArray<TObjectPtr<UWxViewModel_Item>> Retained;

	if (Inventory)
	{
		for (UWxItemInstance* Instance : Inventory->GetAllItems())
		{
			if (!Instance)
			{
				continue;
			}

			UWxViewModel_Item* ChildVM = nullptr;
			for (UWxViewModel_Item* Existing : AllItems)
			{
				if (Existing && Existing->GetTargetInstance() == Instance)
				{
					ChildVM = Existing;
					break;
				}
			}

			if (!ChildVM)
			{
				ChildVM = NewObject<UWxViewModel_Item>(this);
				ChildVM->Initialize(Inventory, Instance);
			}

			NewItems.Add(ChildVM);
			Retained.Add(ChildVM);
		}
	}

	for (UWxViewModel_Item* OldVM : AllItems)
	{
		if (OldVM && !Retained.Contains(OldVM))
		{
			OldVM->Deinitialize();
		}
	}

	AllItems = MoveTemp(NewItems);
	// 슬롯 구성이 그대로여도 ListView 엔트리 UMG 에서 VM 재연결이 가능하도록 항상 브로드캐스트한다.
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AllItems);

	RefreshCategorizedItems();
}

void UWxViewModel_Inventory::SetCurrentCategory(EWxItemCategory NewCategory)
{
	if (CurrentCategory == NewCategory)
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(CurrentCategory, NewCategory);

	RefreshCategorizedItems();
}

void UWxViewModel_Inventory::RefreshCategorizedItems()
{
	TArray<TObjectPtr<UWxViewModel_Item>> NewCategorized;
	NewCategorized.Reserve(AllItems.Num());

	for (UWxViewModel_Item* ItemVM : AllItems)
	{
		if (!ItemVM)
		{
			continue;
		}

		const UWxItemInstance* Instance = ItemVM->GetTargetInstance();
		const UWxItemDefinition* Def = Instance ? Instance->GetItemDef() : nullptr;
		if (Def && Def->GetItemCategory() == CurrentCategory)
		{
			NewCategorized.Add(ItemVM);
		}
	}

	CategorizedItems = MoveTemp(NewCategorized);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CategorizedItems);
}
