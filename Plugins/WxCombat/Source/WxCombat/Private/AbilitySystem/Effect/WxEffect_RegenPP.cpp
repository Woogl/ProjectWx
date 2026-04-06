// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_RegenPP.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_RegenPP::UWxEffect_RegenPP()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Period = FScalableFloat(RegenPeriod);
	bExecutePeriodicEffectOnApplication = true;

	// 가드 중에는 PP 회복 억제
	UTargetTagRequirementsGameplayEffectComponent* TagReqComp = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TargetTagReq"));
	TagReqComp->OngoingTagRequirements.IgnoreTags.AddTag(WxGameplayTags::State_Guard);
	GEComponents.Add(TagReqComp);

	// 틱당 회복량 = MaxPP * (RegenPeriod / FullRegenDuration) = MaxPP / 120
	FAttributeBasedFloat AttributeBased;
	AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxPPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	AttributeBased.Coefficient = FScalableFloat(RegenPeriod / FullRegenDuration);

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetPPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);
	Modifiers.Add(Modifier);
}
