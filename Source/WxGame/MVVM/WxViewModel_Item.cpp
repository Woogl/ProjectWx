// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Item.h"

#include "Blueprint/UserWidget.h"
#include "Controller/WxPlayerController.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"
#include "System/WxInventoryDeveloperSettings.h"

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
	ChargeChangedHandle = InInventory->OnInventoryChargeChanged.AddUObject(this, &UWxViewModel_Item::HandleChargeChanged);

	ApplyStaticDataFromDef(InInstance->GetItemDef());
	RefreshChargeIcon();
	UE_MVVM_SET_PROPERTY_VALUE(TotalCount, InInventory->GetStackCountByInstance(InInstance));
	UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, InInstance->GetCurrentCharges());

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
	ChargeChangedHandle = InInventory->OnInventoryChargeChanged.AddUObject(this, &UWxViewModel_Item::HandleChargeChanged);

	ApplyStaticDataFromDef(InItemDef);
	UE_MVVM_SET_PROPERTY_VALUE(TotalCount, InInventory->GetTotalItemCountByDefinition(InItemDef));
	if (const UWxItemInstance* FirstInstance = InInventory->FindFirstItemStackByDefinition(InItemDef))
	{
		UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, FirstInstance->GetCurrentCharges());
	}
	RefreshChargeIcon();

	SetInitialized(true);
}

void UWxViewModel_Item::Deinitialize()
{
	if (UWxInventoryManagerComponent* Inventory = CachedInventory.Get())
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
	Icon.Reset();
	DisplayName = FText::GetEmpty();
	Grade = EWxItemGrade::Common;
	GradeColor = FLinearColor::White;

	SetInitialized(false);

	Super::Deinitialize();
}

UWxItemInstance* UWxViewModel_Item::GetTargetInstance() const
{
	return TargetInstance.Get();
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

	// 슬롯 모드는 바인딩된 인스턴스, Def 모드는 동일 ItemDef 의 인스턴스 충전 변경만 반영한다.
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
	// 슬롯 모드는 바인딩 인스턴스, Def 모드는 첫 인스턴스를 기준으로 한다(CurrentCharges 와 동일 기준).
	const UWxItemInstance* Instance = TargetInstance.Get();
	if (!Instance)
	{
		const UWxInventoryManagerComponent* Inventory = CachedInventory.Get();
		Instance = Inventory ? Inventory->FindFirstItemStackByDefinition(TargetItemDef.Get()) : nullptr;
	}

	if (Instance)
	{
		UE_MVVM_SET_PROPERTY_VALUE(Icon, Instance->GetDisplayIcon());
	}
}

void UWxViewModel_Item::ApplyStaticDataFromDef(const UWxItemDefinition* InItemDef)
{
	if (!InItemDef)
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(Icon, InItemDef->Icon);
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InItemDef->DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(Grade, InItemDef->Grade);
	UE_MVVM_SET_PROPERTY_VALUE(GradeColor, GetDefault<UWxInventoryDeveloperSettings>()->GetItemGradeColor(InItemDef->Grade));

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

	const AWxPlayerController* PC = Cast<AWxPlayerController>(UserWidget->GetOwningPlayer());
	UWxInventoryManagerComponent* Inventory = PC ? PC->GetInventoryManager() : nullptr;
	if (Inventory)
	{
		ViewModel->Initialize(Inventory, ItemToDisplay);
	}

	return ViewModel;
}
