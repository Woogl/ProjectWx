// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Item.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"

void UWxViewModel_Item::Initialize(UWxInventoryComponent* InInventory, UWxItemInstance* InInstance)
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
	ChargeChangedHandle = InInventory->OnInventoryChargeChanged.AddUObject(this, &UWxViewModel_Item::HandleChargeChanged);

	ApplyStaticDataFromDef(InInstance->GetItemDef());
	RefreshChargeIcon();
	UE_MVVM_SET_PROPERTY_VALUE(TotalCount, InInventory->GetStackCountByInstance(InInstance));
	UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, InInstance->GetCurrentCharges());
}

void UWxViewModel_Item::Initialize(UWxInventoryComponent* InInventory, const UWxItemDefinition* InItemDef)
{
	if (!InInventory || !InItemDef)
	{
		return;
	}

	Deinitialize();

	CachedInventory = InInventory;
	TargetItemDef = InItemDef;
	StackChangedHandle = InInventory->OnInventoryStackChanged.AddUObject(this, &UWxViewModel_Item::HandleStackChanged);
	ChargeChangedHandle = InInventory->OnInventoryChargeChanged.AddUObject(this, &UWxViewModel_Item::HandleChargeChanged);

	ApplyStaticDataFromDef(InItemDef);
	UE_MVVM_SET_PROPERTY_VALUE(TotalCount, InInventory->GetTotalItemCountByDefinition(InItemDef));
	if (const UWxItemInstance* FirstInstance = InInventory->FindFirstItemStackByDefinition(InItemDef))
	{
		UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, FirstInstance->GetCurrentCharges());
	}
	RefreshChargeIcon();
}

void UWxViewModel_Item::Deinitialize()
{
	if (UWxInventoryComponent* Inventory = CachedInventory.Get())
	{
		Inventory->OnInventoryStackChanged.Remove(StackChangedHandle);
		Inventory->OnInventorySlotChanged.Remove(SlotChangedHandle);
		Inventory->OnInventoryChargeChanged.Remove(ChargeChangedHandle);
	}
	StackChangedHandle.Reset();
	SlotChangedHandle.Reset();
	ChargeChangedHandle.Reset();
	CachedInventory.Reset();
	TargetItemDef.Reset();
	TargetInstance.Reset();

	TotalCount = 0;
	CurrentCharges = 0;
	MaxCharges = 0;
	AcquiredCount = 0;
	Icon = nullptr;
	DisplayName = FText::GetEmpty();
	Grade = EWxItemGrade::Common;
	GradeColor = FLinearColor::White;

	Super::Deinitialize();
}

UWxItemInstance* UWxViewModel_Item::GetTargetInstance() const
{
	return TargetInstance.Get();
}

bool UWxViewModel_Item::RequestUseConsumable()
{
	if (UWxInventoryComponent* Inventory = CachedInventory.Get())
	{
		return Inventory->RequestUseConsumable();
	}
	return false;
}

void UWxViewModel_Item::HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta)
{
	if (ItemDef != TargetItemDef.Get())
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(TotalCount, NewCount);
}

void UWxViewModel_Item::HandleSlotChanged(UWxItemInstance* Instance, int32 NewStackCount, int32 Delta)
{
	if (Instance != TargetInstance.Get())
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(TotalCount, NewStackCount);
}

void UWxViewModel_Item::HandleChargeChanged(UWxItemInstance* Instance, int32 NewCharges, int32 Delta)
{
	if (!Instance)
	{
		return;
	}

	const UWxItemInstance* TrackedInstance = TargetInstance.Get();
	const bool bMatches = TrackedInstance ? (Instance == TrackedInstance) : (Instance->GetItemDef() == TargetItemDef.Get());
	if (!bMatches)
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, NewCharges);
	RefreshChargeIcon();
}

void UWxViewModel_Item::RefreshChargeIcon()
{
	const UWxItemInstance* Instance = TargetInstance.Get();
	if (!Instance)
	{
		const UWxInventoryComponent* Inventory = CachedInventory.Get();
		Instance = Inventory ? Inventory->FindFirstItemStackByDefinition(TargetItemDef.Get()) : nullptr;
	}

	if (Instance)
	{
		RequestImageAsync(TEXT("Icon"), Instance->GetDisplayIcon());
	}
}

void UWxViewModel_Item::ApplyLoadedImage(FName FieldName, UObject* LoadedImage)
{
	UE_MVVM_SET_PROPERTY_VALUE(Icon, LoadedImage);
}

void UWxViewModel_Item::ApplyStaticDataFromDef(const UWxItemDefinition* InItemDef)
{
	if (!InItemDef)
	{
		return;
	}

	RequestImageAsync(TEXT("Icon"), InItemDef->Icon);
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InItemDef->DisplayName);

	const UWxItemFragment_Grade* GradeFragment = InItemDef->FindFragmentByClass<UWxItemFragment_Grade>();
	const EWxItemGrade ItemGrade = GradeFragment ? GradeFragment->Grade : EWxItemGrade::Common;
	UE_MVVM_SET_PROPERTY_VALUE(Grade, ItemGrade);
	UE_MVVM_SET_PROPERTY_VALUE(GradeColor, GradeFragment ? GradeFragment->Color : UWxItemFragment_Grade::GetDefaultColorForGrade(EWxItemGrade::Common));

	const UWxItemFragment_Charges* Charges = InItemDef->FindFragmentByClass<UWxItemFragment_Charges>();
	UE_MVVM_SET_PROPERTY_VALUE(MaxCharges, Charges ? Charges->MaxCharges : 0);
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

	UWxInventoryComponent* Inventory = UWxInventoryComponent::FindInventory(UserWidget->GetOwningPlayer());
	if (Inventory)
	{
		ViewModel->Initialize(Inventory, ItemToDisplay);
	}

	return ViewModel;
}
