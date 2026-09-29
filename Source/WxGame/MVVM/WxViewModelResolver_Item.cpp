// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Item.h"
#include "Blueprint/UserWidget.h"
#include "Controller/WxPlayerController.h"
#include "Items/WxItemDefinition.h"
#include "MVVM/WxViewModel_Item.h"

UObject* UWxViewModelResolver_Item::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	AWxPlayerController* PC = UserWidget ? Cast<AWxPlayerController>(UserWidget->GetOwningPlayer()) : nullptr;
	return PC ? PC->GetOrCreateItemViewModel(ItemToDisplay) : nullptr;
}
