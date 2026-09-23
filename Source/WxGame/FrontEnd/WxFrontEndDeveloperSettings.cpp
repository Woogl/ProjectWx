// Copyright Woogle. All Rights Reserved.

#include "FrontEnd/WxFrontEndDeveloperSettings.h"

UWxFrontEndDeveloperSettings::UWxFrontEndDeveloperSettings()
{
	CategoryName = TEXT("Wx");
}

bool UWxFrontEndDeveloperSettings::HasSelectableOptions() const
{
	bool bHasCharacterOption = false;
	for (const TSoftClassPtr<APawn>& Option : CharacterOptions)
	{
		if (!Option.IsNull())
		{
			bHasCharacterOption = true;
			break;
		}
	}

	if (!bHasCharacterOption)
	{
		return false;
	}

	for (const TSoftObjectPtr<UWorld>& Option : LevelOptions)
	{
		if (!Option.IsNull())
		{
			return true;
		}
	}

	return false;
}
