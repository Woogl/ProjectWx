// Copyright Woogle. All Rights Reserved.

#include "WxSaveSettings.h"

#include "Mass/EntityElementTypes.h"
#include "UObject/UObjectIterator.h"

TArray<FString> UWxSaveSettings::GetMassFragmentOptions() const
{
	TArray<FString> Options;
	const UScriptStruct* MassFragmentBase = FMassFragment::StaticStruct();

	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		UScriptStruct* Struct = *It;
		if (Struct != MassFragmentBase && Struct->IsChildOf(MassFragmentBase))
		{
			Options.Add(Struct->GetPathName());
		}
	}

	Options.Sort();
	return Options;
}
