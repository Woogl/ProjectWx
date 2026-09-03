// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_AddAttribute.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxEffect_AddAttribute::UWxEffect_AddAttribute()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
}

void UWxEffect_AddAttribute::AddAttributeModifier(const FGameplayAttribute& Attribute)
{
	FSetByCallerFloat DeltaSetByCaller;
	DeltaSetByCaller.DataTag = WxGameplayTags::SetByCaller_Magnitude;

	FGameplayModifierInfo DeltaModifier;
	DeltaModifier.Attribute = Attribute;
	DeltaModifier.ModifierOp = EGameplayModOp::Additive;
	DeltaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DeltaSetByCaller);
	Modifiers.Add(DeltaModifier);
}

void UWxEffect_AddAttribute::Apply(TSubclassOf<UWxEffect_AddAttribute> EffectClass, UAbilitySystemComponent* TargetASC, float Delta, const FGameplayEffectContextHandle& Context)
{
	if (!EffectClass || !TargetASC)
	{
		return;
	}

	const UGameplayEffect* CDO = EffectClass->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(CDO, Context.IsValid() ? Context : TargetASC->MakeEffectContext(), 1.f);

	Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Magnitude, Delta);
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
}

UWxEffect_AddGP::UWxEffect_AddGP()
{
	AddAttributeModifier(UWxCombatAttributeSet::GetGPAttribute());
}

void UWxEffect_AddGP::Apply(UAbilitySystemComponent* TargetASC, float Delta, const FGameplayEffectContextHandle& Context)
{
	UWxEffect_AddAttribute::Apply(StaticClass(), TargetASC, Delta, Context);
}

UWxEffect_AddIncomingDamage::UWxEffect_AddIncomingDamage()
{
	AddAttributeModifier(UWxCombatAttributeSet::GetIncomingDamageAttribute());
}

void UWxEffect_AddIncomingDamage::Apply(UAbilitySystemComponent* TargetASC, float Delta, const FGameplayEffectContextHandle& Context)
{
	UWxEffect_AddAttribute::Apply(StaticClass(), TargetASC, Delta, Context);
}
