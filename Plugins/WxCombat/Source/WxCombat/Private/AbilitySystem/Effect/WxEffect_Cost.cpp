// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Cost.h"
#include "AbilitySystem/Effect/WxMMC_Cost.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_Cost::UWxEffect_Cost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Additive를 쓰는 이유는 순정 CanApplyAttributeModifiers가 Additive 모디파이어만 보고 자원 부족을 판정하기 때문이다.
	FGameplayModifierInfo MPModifier;
	MPModifier.Attribute = UWxCombatAttributeSet::GetMPAttribute();
	MPModifier.ModifierOp = EGameplayModOp::Additive;
	FCustomCalculationBasedFloat MPCalc;
	MPCalc.CalculationClassMagnitude = UWxMMC_MPCost::StaticClass();
	MPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MPCalc);
	Modifiers.Add(MPModifier);

	FGameplayModifierInfo UPModifier;
	UPModifier.Attribute = UWxCombatAttributeSet::GetUPAttribute();
	UPModifier.ModifierOp = EGameplayModOp::Additive;
	FCustomCalculationBasedFloat UPCalc;
	UPCalc.CalculationClassMagnitude = UWxMMC_UPCost::StaticClass();
	UPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(UPCalc);
	Modifiers.Add(UPModifier);
}
