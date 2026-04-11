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

	TMap<UClass*, TSoftObjectPtr<UTexture2D>> Resolved;
	Resolved.Reserve(SpawnerClassIcons.Num());
	for (const TPair<TSoftClassPtr<AActor>, TSoftObjectPtr<UTexture2D>>& Pair : SpawnerClassIcons)
	{
		if (UClass* Key = Pair.Key.LoadSynchronous())
		{
			Resolved.Add(Key, Pair.Value);
		}
	}

	for (UClass* CurrentClass = ActorClass; CurrentClass && CurrentClass->IsChildOf(AActor::StaticClass()); CurrentClass = CurrentClass->GetSuperClass())
	{
		if (const TSoftObjectPtr<UTexture2D>* FoundIcon = Resolved.Find(CurrentClass))
		{
			return FoundIcon->LoadSynchronous();
		}
	}

	return nullptr;
}
