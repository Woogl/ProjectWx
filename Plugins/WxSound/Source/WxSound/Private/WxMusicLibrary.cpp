// Copyright Woogle. All Rights Reserved.

#include "WxMusicLibrary.h"
#include "System/WxMusicSubsystem.h"
#include "Engine/World.h"

void UWxMusicLibrary::StartBGM(const UObject* WorldContextObject, FGameplayTag BGMTag)
{
	if (!WorldContextObject)
	{
		return;
	}

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UWxMusicSubsystem* Music = World->GetSubsystem<UWxMusicSubsystem>())
		{
			Music->StartBGM(BGMTag);
		}
	}
}

void UWxMusicLibrary::StopBGM(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UWxMusicSubsystem* Music = World->GetSubsystem<UWxMusicSubsystem>())
		{
			Music->StopBGM();
		}
	}
}
