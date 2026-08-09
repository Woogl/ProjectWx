// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Burn.h"
#include "AbilitySystem/Effect/WxExecCalc_Burn.h"
#include "WxGameplayTags.h"

UWxEffect_Burn::UWxEffect_Burn()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(UWxExecCalc_Burn::BurnDuration));

	Period = FScalableFloat(UWxExecCalc_Burn::BurnPeriod);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UWxExecCalc_Burn::StaticClass();
	Executions.Add(ExecDef);

	// TODO: StackingType 직접 대입은 5.7에서 deprecated이나 SetStackingType이 WITH_EDITOR 전용이라 런타임에서 링크가 깨진다.
	// 런타임 setter가 생기면 교체할 것.
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 0;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;

	FGameplayEffectCue Cue;
	Cue.GameplayCueTags.AddTag(WxGameplayTags::GameplayCue_Burn);
	GameplayCues.Add(Cue);
}
