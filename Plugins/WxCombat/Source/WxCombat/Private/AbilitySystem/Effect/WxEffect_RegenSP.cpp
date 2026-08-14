// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_RegenSP.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_RegenSP::UWxEffect_RegenSP()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Period = FScalableFloat(RegenPeriod);
	bExecutePeriodicEffectOnApplication = true;

	UTargetTagRequirementsGameplayEffectComponent* TagReqComp = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TargetTagReq"));
	TagReqComp->OngoingTagRequirements.IgnoreTags.AddTag(WxGameplayTags::Effect_Guard);
	TagReqComp->OngoingTagRequirements.IgnoreTags.AddTag(WxGameplayTags::Movement_Sprint);
	TagReqComp->OngoingTagRequirements.IgnoreTags.AddTag(WxGameplayTags::Effect_Exhausted);
	GEComponents.Add(TagReqComp);

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
