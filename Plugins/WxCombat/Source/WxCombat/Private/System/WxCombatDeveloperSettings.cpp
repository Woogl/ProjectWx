// Copyright Woogle. All Rights Reserved.

#include "System/WxCombatDeveloperSettings.h"

UWxCombatDeveloperSettings::UWxCombatDeveloperSettings()
{
	CategoryName = TEXT("Wx");
	DefenseConstant = 100.f;
	CombatAnimNotifyColor = FLinearColor::Red;
	MovementAnimNotifyColor = FLinearColor::Blue;
	CosmeticAnimNotifyColor = FLinearColor::Green;
}
