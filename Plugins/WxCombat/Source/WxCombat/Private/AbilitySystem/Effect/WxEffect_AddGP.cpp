// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_AddGP.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxEffect_AddGP::UWxEffect_AddGP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DPSetByCaller;
	DPSetByCaller.DataTag = WxGameplayTags::SetByCaller_DP;

	FGameplayModifierInfo DPModifier;
	DPModifier.Attribute = UWxCombatAttributeSet::GetGPAttribute();
	DPModifier.ModifierOp = EGameplayModOp::Additive;
	DPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DPSetByCaller);
	Modifiers.Add(DPModifier);
}

void UWxEffect_AddGP::ApplyTo(UAbilitySystemComponent* TargetASC, float Amount)
{
	if (!TargetASC || Amount <= 0.f)
	{
		return;
	}

	const UGameplayEffect* CDO = StaticClass()->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), 1.f);
	Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_DP, Amount);
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
}
