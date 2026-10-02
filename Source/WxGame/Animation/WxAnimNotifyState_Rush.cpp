// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotifyState_Rush.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotifyState_Rush::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->MovementColor;
}
#endif

FString UWxAnimNotifyState_Rush::GetNotifyName_Implementation() const
{
	switch (TargetSource)
	{
	case EWxRushTarget::LockOnTarget:
		return TEXT("Rush: LockOn");
	case EWxRushTarget::Master:
		return TEXT("Rush: Master");
	case EWxRushTarget::Minion:
		return TEXT("Rush: Minion");
	default:
		return TEXT("Rush: None");
	}
}
