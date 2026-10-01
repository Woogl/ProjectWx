// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotifyState_SlowTime.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotifyState_SlowTime::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->PresentationColor;
}
#endif

FString UWxAnimNotifyState_SlowTime::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Slow: x%.2f"), TimeDilation);
}
