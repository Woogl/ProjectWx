// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_BuffATK.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_BuffATK::UWxEffect_BuffATK()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.f));

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetATKAttribute();
	Modifier.ModifierOp = EGameplayModOp::Multiplicitive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.5f));
	Modifiers.Add(Modifier);

	FGameplayEffectCue Cue;
	Cue.GameplayCueTags.AddTag(WxGameplayTags::GameplayCue_BuffATK);
	GameplayCues.Add(Cue);
}
