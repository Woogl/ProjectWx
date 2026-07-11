// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Selection.h"

void UWxViewModel_Selection::SetSelection(const FText& InDisplayName, const FText& InDescription, const TSoftObjectPtr<UTexture2D>& InIcon)
{
	if (!bHasSelection)
	{
		bHasSelection = true;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bHasSelection);
	}

	if (!DisplayName.IdenticalTo(InDisplayName))
	{
		DisplayName = InDisplayName;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DisplayName);
	}

	if (!Description.IdenticalTo(InDescription))
	{
		Description = InDescription;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Description);
	}

	if (Icon != InIcon)
	{
		Icon = InIcon;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Icon);
	}
}

void UWxViewModel_Selection::ClearSelection()
{
	if (bHasSelection)
	{
		bHasSelection = false;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bHasSelection);
	}

	if (!DisplayName.IsEmpty())
	{
		DisplayName = FText::GetEmpty();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DisplayName);
	}

	if (!Description.IsEmpty())
	{
		Description = FText::GetEmpty();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Description);
	}

	if (!Icon.IsNull())
	{
		Icon = nullptr;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Icon);
	}
}
