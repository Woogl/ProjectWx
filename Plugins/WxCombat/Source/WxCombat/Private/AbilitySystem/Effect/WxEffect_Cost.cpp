// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Cost.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
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

	FGameplayModifierInfo SPModifier;
	SPModifier.Attribute = UWxCombatAttributeSet::GetSPAttribute();
	SPModifier.ModifierOp = EGameplayModOp::Additive;
	FCustomCalculationBasedFloat SPCalc;
	SPCalc.CalculationClassMagnitude = UWxMMC_SPCost::StaticClass();
	SPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SPCalc);
	Modifiers.Add(SPModifier);
}

float UWxMMC_Cost::GetCostMagnitude(const FGameplayEffectSpec& Spec, EWxAbilityCostResource Resource) const
{
	// 컨텍스트의 소스 어빌리티 CDO는 AbilityDataRow를 그대로 가진다(EditDefaultsOnly).
	// 정적 데이터라 서버/클라 동일.
	const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.GetEffectContext().GetAbility());
	if (!Ability || Ability->AbilityDataRow.IsNull())
	{
		return 0.f;
	}

	const FWxAbilityTableRow* Row = Ability->AbilityDataRow.GetRow<FWxAbilityTableRow>(TEXT("UWxMMC_Cost::GetCostMagnitude"));
	return Row && Row->CostResource == Resource ? -Row->CostAmount : 0.f;
}

float UWxMMC_MPCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return GetCostMagnitude(Spec, EWxAbilityCostResource::MP);
}

float UWxMMC_UPCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return GetCostMagnitude(Spec, EWxAbilityCostResource::UP);
}

float UWxMMC_SPCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return GetCostMagnitude(Spec, EWxAbilityCostResource::SP);
}
