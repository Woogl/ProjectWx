// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_SpawnMinion.h"
#include "GameFramework/Pawn.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotify_SpawnMinion::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->MiscColor;
}
#endif

UWxAnimNotify_SpawnMinion::UWxAnimNotify_SpawnMinion()
{
	LocalSpawnOffset.SetLocation(FVector(200.0f, 0.0f, 0.0f));
}

FString UWxAnimNotify_SpawnMinion::GetNotifyName_Implementation() const
{
	FString ClassName = MinionClass ? MinionClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Summon: %s"), *ClassName);
}
