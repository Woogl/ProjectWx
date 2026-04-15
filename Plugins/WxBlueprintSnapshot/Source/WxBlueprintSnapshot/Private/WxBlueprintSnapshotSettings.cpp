// Copyright Woogle. All Rights Reserved.

#include "WxBlueprintSnapshotSettings.h"

UWxBlueprintSnapshotSettings::UWxBlueprintSnapshotSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("WxBlueprintSnapshot");
}

FName UWxBlueprintSnapshotSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}
