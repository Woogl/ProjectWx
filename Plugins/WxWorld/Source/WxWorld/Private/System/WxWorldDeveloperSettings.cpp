// Copyright Woogle. All Rights Reserved.

#include "System/WxWorldDeveloperSettings.h"

UWxWorldDeveloperSettings::UWxWorldDeveloperSettings()
{
	CategoryName = TEXT("Wx");
}

UTexture2D* UWxWorldDeveloperSettings::FindSpawnerIconForClass(UClass* ActorClass) const
{
	if (!ActorClass)
	{
		return nullptr;
	}

	for (UClass* CurrentClass = ActorClass; CurrentClass && CurrentClass->IsChildOf(AActor::StaticClass()); CurrentClass = CurrentClass->GetSuperClass())
	{
		const TSoftClassPtr<AActor> CurrentClassPtr(CurrentClass);
		if (const TSoftObjectPtr<UTexture2D>* FoundIcon = SpawnerClassIcons.Find(CurrentClassPtr))
		{
			return FoundIcon->LoadSynchronous();
		}
	}

	return nullptr;
}
