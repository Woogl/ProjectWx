// Copyright Woogle. All Rights Reserved.

#include "FrontEnd/WxFrontEndDeveloperSettings.h"

UWxFrontEndDeveloperSettings::UWxFrontEndDeveloperSettings()
{
	CategoryName = TEXT("Wx");
}

bool UWxFrontEndDeveloperSettings::HasSelectableOptions() const
{
	return CharacterOptions.ContainsByPredicate([](const TSoftClassPtr<APawn>& Option) { return !Option.IsNull(); })
		&& LevelOptions.ContainsByPredicate([](const TSoftObjectPtr<UWorld>& Option) { return !Option.IsNull(); });
}
