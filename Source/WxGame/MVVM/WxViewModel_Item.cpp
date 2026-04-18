// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Item.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Player/WxPlayerState.h"

void UWxViewModel_Item::Initialize(UWxInventoryManagerComponent* InInventory, const UWxItemDefinition* InItemDef)
{
	if (!InInventory || !InItemDef)
	{
		return;
	}

	Deinitialize();

	CachedInventory = InInventory;
	TargetItemDef = InItemDef;
	StackChangedHandle = InInventory->OnInventoryStackChanged.AddUObject(this, &UWxViewModel_Item::HandleStackChanged);

	UE_MVVM_SET_PROPERTY_VALUE(ItemCount, InInventory->GetItemCountByDef(InItemDef));
	UE_MVVM_SET_PROPERTY_VALUE(LastDelta, 0);
	UE_MVVM_SET_PROPERTY_VALUE(Icon, InItemDef->Icon.LoadSynchronous());
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InItemDef->DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(Grade, InItemDef->Grade);

	SetInitialized(true);
}

void UWxViewModel_Item::Deinitialize()
{
	if (UWxInventoryManagerComponent* Inventory = CachedInventory.Get())
	{
		Inventory->OnInventoryStackChanged.Remove(StackChangedHandle);
	}
	StackChangedHandle.Reset();
	CachedInventory.Reset();
	TargetItemDef.Reset();

	ItemCount = 0;
	LastDelta = 0;
	Icon = nullptr;
	DisplayName = FText::GetEmpty();
	Grade = EWxItemGrade::Common;

	SetInitialized(false);

	Super::Deinitialize();
}

void UWxViewModel_Item::HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta)
{
	if (ItemDef != TargetItemDef.Get())
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(ItemCount, NewCount);
	UE_MVVM_SET_PROPERTY_VALUE(LastDelta, Delta);
}

UObject* UWxViewModelResolver_Item::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (!UserWidget || !ExpectedType)
	{
		return nullptr;
	}

	UWxViewModel_Item* ViewModel = NewObject<UWxViewModel_Item>(const_cast<UUserWidget*>(UserWidget), ExpectedType);

	if (!ItemToDisplay)
	{
		return ViewModel;
	}

	const APlayerController* PC = UserWidget->GetOwningPlayer();
	const AWxPlayerState* PS = PC ? PC->GetPlayerState<AWxPlayerState>() : nullptr;
	UWxInventoryManagerComponent* Inventory = PS ? PS->GetInventoryManager() : nullptr;
	if (Inventory)
	{
		ViewModel->Initialize(Inventory, ItemToDisplay);
	}

	return ViewModel;
}
