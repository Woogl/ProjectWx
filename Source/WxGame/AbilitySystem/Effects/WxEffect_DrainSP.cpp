// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effects/WxEffect_DrainSP.h"
#include "AbilitySystem/Attributes/WxCombatAttributeSet.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_DrainSP::UWxEffect_DrainSP()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Period = FScalableFloat(DrainPeriod);
	bExecutePeriodicEffectOnApplication = true;

	UTargetTagRequirementsGameplayEffectComponent* TagReqComp = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TargetTagReq"));
	TagReqComp->OngoingTagRequirements.RequireTags.AddTag(WxGameplayTags::Movement_Sprint);
	GEComponents.Add(TagReqComp);

	FAttributeBasedFloat AttributeBased;
	AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxSPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	AttributeBased.Coefficient = FScalableFloat(-DrainPeriod / FullDrainDuration);

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetSPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);
	Modifiers.Add(Modifier);
}
