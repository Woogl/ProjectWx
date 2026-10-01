// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_StartRecovery.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotify_StartRecovery::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->AbilityFlowColor;
}
#endif

FString UWxAnimNotify_StartRecovery::GetNotifyName_Implementation() const
{
	return TEXT("Recovery");
}
