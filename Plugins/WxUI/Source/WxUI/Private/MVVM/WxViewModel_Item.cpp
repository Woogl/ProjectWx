// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Item.h"
#include "MVVM/WxViewModelUtils.h"

void UWxViewModel_Item::SetSourceObject(const UObject* InSourceObject)
{
	UE_MVVM_SET_PROPERTY_VALUE(SourceObject, InSourceObject);
}

void UWxViewModel_Item::SetDisplayName(const FText& InDisplayName)
{
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InDisplayName);
}

void UWxViewModel_Item::SetIcon(const TSoftObjectPtr<UObject>& InIcon)
{
	WxViewModel::RequestImageAsync(*this, IconHandle, InIcon, [this](UObject* LoadedIcon)
	{
		UE_MVVM_SET_PROPERTY_VALUE(Icon, LoadedIcon);
	});
}

void UWxViewModel_Item::SetTotalCount(int32 InTotalCount)
{
	UE_MVVM_SET_PROPERTY_VALUE(TotalCount, InTotalCount);
}

void UWxViewModel_Item::SetCurrentCharges(int32 InCurrentCharges)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, InCurrentCharges);
}

void UWxViewModel_Item::SetGradeColor(const FLinearColor& InGradeColor)
{
	UE_MVVM_SET_PROPERTY_VALUE(GradeColor, InGradeColor);
}
