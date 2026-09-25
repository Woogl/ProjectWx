// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Combo.h"
#include "Animation/AnimMontage.h"

UAnimMontage* UWxAbility_Combo::GetMontage() const
{
	const int32 MontageIndex = ComboIndex == INDEX_NONE ? 0 : ComboIndex;
	return ComboMontages.IsValidIndex(MontageIndex) ? ComboMontages[MontageIndex].Get() : nullptr;
}
