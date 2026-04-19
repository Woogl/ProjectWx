// Copyright Woogle. All Rights Reserved.

#include "WxBlueprintSnapshotSettings.h"

UWxBlueprintSnapshotSettings::UWxBlueprintSnapshotSettings()
{
	CategoryName = TEXT("Wx");
	SectionName = TEXT("WxBlueprintSnapshot");
	OutputDirectory.Path = TEXT("Plugins/WxBlueprintSnapshot/Snapshots");
}
