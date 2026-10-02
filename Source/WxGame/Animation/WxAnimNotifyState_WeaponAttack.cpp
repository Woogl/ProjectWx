// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotifyState_WeaponAttack.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotifyState_WeaponAttack::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->AttackColor;
}
#endif

FString UWxAnimNotifyState_WeaponAttack::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Attack: %s"), DamageDataRow.IsNull() ? TEXT("None") : *DamageDataRow.RowName.ToString());
}
