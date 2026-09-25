// Copyright Woogle. All Rights Reserved.

#include "WxAnimNotifySettings.h"

UWxAnimNotifySettings::UWxAnimNotifySettings()
{
	CategoryName = TEXT("Wx");
#if WITH_EDITORONLY_DATA
	// 팔레트의 HEX는 sRGB 값이므로 선형 색상으로 변환한다.
	AttackColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E86666")));
	AbilityFlowColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E8BE55")));
	EffectColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("B58AE6")));
	MovementColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("62A9E8")));
	PresentationColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("63C49A")));
	MiscColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("929DAA")));
#endif
}
