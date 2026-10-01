// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModelResolver_Item.h"
#include "Blueprint/UserWidget.h"
#include "UI/MVVM/WxViewModel_Inventory.h"
#include "UI/MVVM/WxViewModel_Item.h"

UObject* UWxViewModelResolver_Item::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UWxViewModel_Inventory* InventoryViewModel = UWxViewModel_Inventory::FindPlayer(UserWidget);
	return InventoryViewModel ? InventoryViewModel->GetOrCreateItemViewModel(ItemToDisplay) : nullptr;
}
