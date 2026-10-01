// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotifyState_ApplyGameplayEffect.h"
#include "GameplayEffect.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotifyState_ApplyGameplayEffect::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->EffectColor;
}
#endif

FString UWxAnimNotifyState_ApplyGameplayEffect::GetNotifyName_Implementation() const
{
	FString ClassName = EffectClass ? EffectClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Effect: %s"), *ClassName);
}
