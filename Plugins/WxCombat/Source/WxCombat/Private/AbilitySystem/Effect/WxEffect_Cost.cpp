// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Cost.h"
#include "AbilitySystem/Effect/WxMMC_Cost.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_Cost::UWxEffect_Cost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// MP 코스트. 값은 UWxMMC_MPCost가 소스 어빌리티의 AbilityDataRow(MPCost)에서 조회한다.
	// ModifierOp=Additive: 순정 CanApplyAttributeModifiers가 Additive 모디파이어만 자원 부족을 판정하므로 검사 대상과 일치시킨다.
	FGameplayModifierInfo MPModifier;
	MPModifier.Attribute = UWxCombatAttributeSet::GetMPAttribute();
	MPModifier.ModifierOp = EGameplayModOp::Additive;
	FCustomCalculationBasedFloat MPCalc;
	MPCalc.CalculationClassMagnitude = UWxMMC_MPCost::StaticClass();
	MPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MPCalc);
	Modifiers.Add(MPModifier);

	// UP 코스트. 값은 UWxMMC_UPCost가 AbilityDataRow(UPCost)에서 조회한다.
	FGameplayModifierInfo UPModifier;
	UPModifier.Attribute = UWxCombatAttributeSet::GetUPAttribute();
	UPModifier.ModifierOp = EGameplayModOp::Additive;
	FCustomCalculationBasedFloat UPCalc;
	UPCalc.CalculationClassMagnitude = UWxMMC_UPCost::StaticClass();
	UPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(UPCalc);
	Modifiers.Add(UPModifier);
}
