// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModelResolver_Item.h"
#include "Blueprint/UserWidget.h"
#include "Player/WxPlayerController.h"
#include "Inventory/WxItemDefinition.h"
#include "UI/MVVM/WxViewModel_Item.h"

UObject* UWxViewModelResolver_Item::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	AWxPlayerController* PC = UserWidget ? Cast<AWxPlayerController>(UserWidget->GetOwningPlayer()) : nullptr;
	return PC ? PC->GetOrCreateItemViewModel(ItemToDisplay) : nullptr;
}
