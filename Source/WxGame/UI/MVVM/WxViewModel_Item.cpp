// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_Item.h"
#include "Inventory/WxInventoryComponent.h"
#include "Inventory/WxItemDefinition.h"
#include "Inventory/WxItemFragment.h"
#include "Inventory/WxItemInstance.h"
#include "UI/MVVM/WxViewModelUtils.h"

void UWxViewModel_Item::Initialize(const UWxItemDefinition& InDefinition, const UWxItemInstance* InInstance)
{
	Definition = &InDefinition;
	Instance = InInstance;

	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InDefinition.DisplayName);
	UE_MVVM_SET_PROPERTY_VALUE(Category, InDefinition.GetItemCategory());
	const UWxItemFragment_Grade* GradeFragment = InDefinition.FindFragmentByClass<UWxItemFragment_Grade>();
	UE_MVVM_SET_PROPERTY_VALUE(GradeColor, GradeFragment ? GradeFragment->Color : UWxItemFragment_Grade::GetDefaultColorForGrade(EWxItemGrade::Common));
}

void UWxViewModel_Item::Refresh(const UWxInventoryComponent& Inventory)
{
	const int32 NewTotalCount = Instance ? Inventory.GetStackCountByInstance(Instance) : Inventory.GetTotalItemCountByDefinition(Definition);
	const UWxItemInstance* DisplayInstance = Instance ? Instance.Get() : Inventory.FindFirstItemStackByDefinition(Definition);
	const bool bHasStack = DisplayInstance && NewTotalCount > 0;

	UE_MVVM_SET_PROPERTY_VALUE(TotalCount, NewTotalCount);
	UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, bHasStack ? DisplayInstance->GetCurrentCharges() : 0);
	SetIcon(bHasStack ? DisplayInstance->GetDisplayIcon() : Definition->Icon);
}

const UWxItemDefinition* UWxViewModel_Item::GetDefinition() const
{
	return Definition;
}

const UWxItemInstance* UWxViewModel_Item::GetInstance() const
{
	return Instance;
}

void UWxViewModel_Item::SetIcon(const TSoftObjectPtr<UObject>& InIcon)
{
	WxViewModel::RequestImageAsync(*this, IconHandle, InIcon, [this](UObject* LoadedIcon)
	{
		UE_MVVM_SET_PROPERTY_VALUE(Icon, LoadedIcon);
	});
}
