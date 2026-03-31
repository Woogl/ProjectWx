// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Bleed.h"
#include "AbilitySystem/Effect/WxExecCalc_Bleed.h"
#include "WxGameplayTags.h"

UWxEffect_Bleed::UWxEffect_Bleed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(UWxExecCalc_Bleed::BleedDuration));

	Period = FScalableFloat(UWxExecCalc_Bleed::BleedPeriod);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UWxExecCalc_Bleed::StaticClass();
	Executions.Add(ExecDef);

	// TODO: StackingType 직접 대입은 UE 5.7에서 deprecated. SetStackingType()은 WITH_EDITOR 전용이라 런타임 빌드에서 링크 실패.
	//       엔진 업데이트로 런타임 setter가 추가되면 교체할 것.
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
