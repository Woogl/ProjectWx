// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_RegenSP.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_RegenSP::UWxEffect_RegenSP()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Period = FScalableFloat(RegenPeriod);
	bExecutePeriodicEffectOnApplication = true;

	// 가드 중에는 SP 회복 억제
	UTargetTagRequirementsGameplayEffectComponent* TagReqComp = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TargetTagReq"));
	TagReqComp->OngoingTagRequirements.IgnoreTags.AddTag(WxGameplayTags::State_Guard);
	GEComponents.Add(TagReqComp);

	// 틱당 회복량 = MaxSP * (RegenPeriod / FullRegenDuration) = MaxSP / 120
	FAttributeBasedFloat AttributeBased;
	AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxSPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	AttributeBased.Coefficient = FScalableFloat(RegenPeriod / FullRegenDuration);

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetSPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);
	Modifiers.Add(Modifier);
}
