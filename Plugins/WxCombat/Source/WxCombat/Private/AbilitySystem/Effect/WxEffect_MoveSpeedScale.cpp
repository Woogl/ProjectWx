// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_MoveSpeedScale.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_MoveSpeedScale::UWxEffect_MoveSpeedScale()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FSetByCallerFloat ScaleSetByCaller;
	ScaleSetByCaller.DataTag = WxGameplayTags::SetByCaller_MoveSpeedScale;

	FGameplayModifierInfo ScaleModifier;
	ScaleModifier.Attribute = UWxCombatAttributeSet::GetSPDAttribute();
	// MultiplyAdditive 는 여러 배율을 더해서 합치므로(0.5 두 개 = 0.0) 감속이 겹치면 폰이 멈춘다.
	ScaleModifier.ModifierOp = EGameplayModOp::MultiplyCompound;
	ScaleModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(ScaleSetByCaller);
	Modifiers.Add(ScaleModifier);
}
