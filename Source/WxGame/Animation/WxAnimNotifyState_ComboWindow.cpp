// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotifyState_ComboWindow.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotifyState_ComboWindow::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->AbilityFlowColor;
}
#endif

FString UWxAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("Combo Window");
}
