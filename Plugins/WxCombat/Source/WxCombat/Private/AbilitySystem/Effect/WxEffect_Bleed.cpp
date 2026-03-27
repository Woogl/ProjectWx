// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Bleed.h"
#include "AbilitySystem/Effect/WxBleedExecCalc.h"
#include "WxGameplayTags.h"

UWxEffect_Bleed::UWxEffect_Bleed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(UWxBleedExecCalc::BleedDuration));

	Period = FScalableFloat(UWxBleedExecCalc::BleedPeriod);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UWxBleedExecCalc::StaticClass();
	Executions.Add(ExecDef);

PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 0;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;

	FGameplayEffectCue Cue;
	Cue.GameplayCueTags.AddTag(WxGameplayTags::GameplayCue_Bleed);
	GameplayCues.Add(Cue);
}
