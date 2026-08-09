// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Exceed.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_Exceed::UWxEffect_Exceed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(6.f));

	// StackingType은 5.7에서 deprecated지만 SetStackingType이 WITH_EDITOR 한정이라 생성자에서 못 쓰므로 직접 대입한다.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	FGameplayModifierInfo AtkMod;
	AtkMod.Attribute = UWxCombatAttributeSet::GetATKAttribute();
	AtkMod.ModifierOp = EGameplayModOp::MultiplyAdditive;
	AtkMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.2f));
	Modifiers.Add(AtkMod);

	FGameplayModifierInfo AspdMod;
	AspdMod.Attribute = UWxCombatAttributeSet::GetASPDAttribute();
	AspdMod.ModifierOp = EGameplayModOp::MultiplyAdditive;
	AspdMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.2f));
	Modifiers.Add(AspdMod);

	FGameplayEffectCue Cue;
	Cue.GameplayCueTags.AddTag(WxGameplayTags::GameplayCue_Exceed);
	GameplayCues.Add(Cue);
}
