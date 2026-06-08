// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_HealByMaxHPPercent.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_HealByMaxHPPercent::UWxEffect_HealByMaxHPPercent()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// MaxHP의 40%를 가산 회복. HP는 어트리뷰트 세트에서 [0, MaxHP]로 클램프되므로 오버힐은 발생하지 않는다.
	FAttributeBasedFloat MaxHPBased;
	MaxHPBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxHPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	MaxHPBased.Coefficient = 0.4f;
	MaxHPBased.PreMultiplyAdditiveValue = 0.f;
	MaxHPBased.PostMultiplyAdditiveValue = 0.f;

	FGameplayModifierInfo HPModifier;
	HPModifier.Attribute = UWxCombatAttributeSet::GetHPAttribute();
	HPModifier.ModifierOp = EGameplayModOp::Additive;
	HPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MaxHPBased);
	Modifiers.Add(HPModifier);
}
