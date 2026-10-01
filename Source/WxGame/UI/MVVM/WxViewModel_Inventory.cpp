// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_Inventory.h"

#include "UI/MVVM/WxViewModel_Item.h"
#include "UI/MVVM/WxViewModelUtils.h"
#include "Types/MVVMViewModelCollection.h"
#include "WxGameplayTags.h"

UWxViewModel_Inventory::UWxViewModel_Inventory()
{
	CurrentCategory = WxGameplayTags::Item_Category_Equipment;
}

FMVVMViewModelContext UWxViewModel_Inventory::GetPlayerContext()
{
	FMVVMViewModelContext Context;
	Context.ContextClass = StaticClass();
	Context.ContextName = TEXT("VM_Inventory");
	return Context;
}

UWxViewModel_Inventory* UWxViewModel_Inventory::FindPlayer(const UObject* WorldContextObject)
{
	UMVVMViewModelCollectionObject* Collection = WxViewModel::GetGlobalCollection(WorldContextObject);
	return Collection ? Cast<UWxViewModel_Inventory>(Collection->FindViewModelInstance(GetPlayerContext())) : nullptr;
}

void UWxViewModel_Inventory::SetItems(TArray<TObjectPtr<UWxViewModel_Item>> InItems)
{
	Items = MoveTemp(InItems);
	RefreshCategorizedItems();
}

const TArray<TObjectPtr<UWxViewModel_Item>>& UWxViewModel_Inventory::GetItems() const
{
	return Items;
}

UWxViewModel_Item* UWxViewModel_Inventory::FindItemViewModel(const UObject* Source) const
{
	for (UWxViewModel_Item* Existing : ItemViewModels)
	{
		if (Existing && Existing->SourceObject == Source)
		{
			return Existing;
		}
	}
	return nullptr;
}

void UWxViewModel_Inventory::AddItemViewModel(UWxViewModel_Item* ItemViewModel)
{
	ItemViewModels.Add(ItemViewModel);
}

const TArray<TObjectPtr<UWxViewModel_Item>>& UWxViewModel_Inventory::GetItemViewModels() const
{
	return ItemViewModels;
}

void UWxViewModel_Inventory::SetLastAcquiredItem(UWxViewModel_Item* InLastAcquiredItem)
{
	UE_MVVM_SET_PROPERTY_VALUE(LastAcquiredItem, InLastAcquiredItem);
}

void UWxViewModel_Inventory::SetCurrentCategory(FGameplayTag NewCategory)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(CurrentCategory, NewCategory))
	{
		RefreshCategorizedItems();
	}
}

void UWxViewModel_Inventory::RefreshCategorizedItems()
{
	TArray<TObjectPtr<UWxViewModel_Item>> NewCategorized;
	NewCategorized.Reserve(Items.Num());

	for (UWxViewModel_Item* ItemViewModel : Items)
	{
		if (ItemViewModel && ItemViewModel->Category.MatchesTagExact(CurrentCategory))
		{
			NewCategorized.Add(ItemViewModel);
		}
	}

	CategorizedItems = MoveTemp(NewCategorized);
	// 구성이 그대로여도 ListView 엔트리 UMG 에서 VM 재연결이 가능하도록 항상 브로드캐스트한다.
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CategorizedItems);
}
