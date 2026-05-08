// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxEffect_RecoverResource::UWxEffect_RecoverResource()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat UPSetByCaller;
	UPSetByCaller.DataTag = WxGameplayTags::SetByCaller_Recovery_UP;

	FGameplayModifierInfo UPModifier;
	UPModifier.Attribute = UWxCombatAttributeSet::GetUPAttribute();
	UPModifier.ModifierOp = EGameplayModOp::Additive;
	UPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(UPSetByCaller);
	Modifiers.Add(UPModifier);

	FSetByCallerFloat MPSetByCaller;
	MPSetByCaller.DataTag = WxGameplayTags::SetByCaller_Recovery_MP;

	FGameplayModifierInfo MPModifier;
	MPModifier.Attribute = UWxCombatAttributeSet::GetMPAttribute();
	MPModifier.ModifierOp = EGameplayModOp::Additive;
	MPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MPSetByCaller);
	Modifiers.Add(MPModifier);
}

void UWxEffect_RecoverResource::ApplyTo(UAbilitySystemComponent* TargetASC, float UP, float MP)
{
	if (!TargetASC || (UP <= 0.f && MP <= 0.f))
	{
		return;
	}

	const UGameplayEffect* CDO = StaticClass()->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), 1.f);
	Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_UP, FMath::Max(UP, 0.f));
	Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_MP, FMath::Max(MP, 0.f));
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
}
