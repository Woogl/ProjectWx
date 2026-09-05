// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Item.h"

void UWxViewModel_Item::Initialize(const UObject* InSourceObject, FText InDisplayName, const TSoftObjectPtr<UObject>& InIcon)
{
	Deinitialize();
	SetSourceObject(InSourceObject);
	SetDisplayName(InDisplayName);
	SetIcon(InIcon);
}

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
	RequestImageAsync(TEXT("Icon"), InIcon);
}

void UWxViewModel_Item::Deinitialize()
{
	Super::Deinitialize();
	if (HasAnyFlags(RF_BeginDestroyed))
	{
		SourceObject = nullptr;
		DisplayName = FText::GetEmpty();
		Icon = nullptr;
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(SourceObject, nullptr);
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, FText::GetEmpty());
	UE_MVVM_SET_PROPERTY_VALUE(Icon, nullptr);
}

void UWxViewModel_Item::ApplyLoadedImage(FName FieldName, UObject* LoadedImage)
{
	if (FieldName == TEXT("Icon"))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Icon, LoadedImage);
	}
}
