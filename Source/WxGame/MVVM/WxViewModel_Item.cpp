// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Item.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemInstance.h"
#include "Player/WxPlayerState.h"

void UWxViewModel_Item::Initialize(UWxInventoryManagerComponent* InInventory, UWxItemInstance* InInstance)
{
	if (!InInventory || !InInstance)
	{
		return;
	}

	Deinitialize();

	CachedInventory = InInventory;
	TargetInstance = InInstance;
	TargetItemDef = InInstance->GetItemDef();
	SlotChangedHandle = InInventory->OnInventorySlotChanged.AddUObject(this, &UWxViewModel_Item::HandleSlotChanged);

	ApplyStaticDataFromDef(InInstance->GetItemDef());
	UE_MVVM_SET_PROPERTY_VALUE(ItemCount, InInventory->GetStackCountByInstance(InInstance));
	UE_MVVM_SET_PROPERTY_VALUE(LastDelta, 0);

	SetInitialized(true);
}

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

	ApplyStaticDataFromDef(InItemDef);
	UE_MVVM_SET_PROPERTY_VALUE(ItemCount, InInventory->GetItemCountByDef(InItemDef));
	UE_MVVM_SET_PROPERTY_VALUE(LastDelta, 0);

	SetInitialized(true);
}

void UWxViewModel_Item::Deinitialize()
{
	if (UWxInventoryManagerComponent* Inventory = CachedInventory.Get())
	{
		Inventory->OnInventoryStackChanged.Remove(StackChangedHandle);
		Inventory->OnInventorySlotChanged.Remove(SlotChangedHandle);
	}
	StackChangedHandle.Reset();
	SlotChangedHandle.Reset();
	CachedInventory.Reset();
	TargetItemDef.Reset();
	TargetInstance.Reset();

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

UWxItemInstance* UWxViewModel_Item::GetTargetInstance() const
{
	return TargetInstance.Get();
}

void UWxViewModel_Item::HandleSlotChanged(const UWxItemInstance* Instance, int32 NewStackCount, int32 Delta)
{
	if (Instance != TargetInstance.Get())
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(ItemCount, NewStackCount);
	UE_MVVM_SET_PROPERTY_VALUE(LastDelta, Delta);
}

void UWxViewModel_Item::ApplyStaticDataFromDef(const UWxItemDefinition* InItemDef)
{
	if (!InItemDef)
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(Icon, InItemDef->Icon.LoadSynchronous());
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InItemDef->DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(Grade, InItemDef->Grade);
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
