// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_DespawnMinion.h"
#include "GameFramework/Pawn.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotify_DespawnMinion::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->MiscColor;
}
#endif

FString UWxAnimNotify_DespawnMinion::GetNotifyName_Implementation() const
{
	FString ClassName = MinionClass ? MinionClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Despawn: %s"), *ClassName);
}
