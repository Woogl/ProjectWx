// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Inventory.h"

#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"

void UWxViewModel_Inventory::Initialize(UWxInventoryManagerComponent* InInventory)
{
	if (!InInventory)
	{
		return;
	}

	Deinitialize();

	CachedInventory = InInventory;
	StackChangedHandle = InInventory->OnInventoryStackChanged.AddUObject(this, &UWxViewModel_Inventory::HandleStackChanged);

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

	LastChangedCurrency = FGameplayTag();
	LastChangedAmount = 0;
	LastChangedDelta = 0;

	SetInitialized(false);

	Super::Deinitialize();
}

int32 UWxViewModel_Inventory::GetCurrencyAmount(FGameplayTag CurrencyTag) const
{
	const UWxInventoryManagerComponent* Inventory = CachedInventory.Get();
	return Inventory ? Inventory->GetItemCountByTag(CurrencyTag) : 0;
}

void UWxViewModel_Inventory::HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta)
{
	FGameplayTag ChangedTag;
	if (ItemDef)
	{
		if (const FWxItemFragment_Currency* Currency = ItemDef->FindFragment<FWxItemFragment_Currency>())
		{
			ChangedTag = Currency->CurrencyTag;
		}
	}

	UE_MVVM_SET_PROPERTY_VALUE(LastChangedCurrency, ChangedTag);
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedAmount, NewCount);
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedDelta, Delta);
}
