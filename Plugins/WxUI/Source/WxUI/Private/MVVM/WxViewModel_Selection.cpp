// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Selection.h"

void UWxViewModel_Selection::SetSelection(const FText& InDisplayName, const FText& InDescription, const TSoftObjectPtr<UObject>& InIcon)
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

	// 로드 완료 시점에 ApplyLoadedImage 가 Icon 을 세팅하고 발화한다.
	RequestImageAsync(TEXT("Icon"), InIcon);
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

	// 진행 중인 스트리밍이 있으면 함께 취소된다.
	RequestImageAsync(TEXT("Icon"), nullptr);
}

void UWxViewModel_Selection::ApplyLoadedImage(FName FieldName, UObject* LoadedImage)
{
	if (Icon != LoadedImage)
	{
		Icon = LoadedImage;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Icon);
	}
}
