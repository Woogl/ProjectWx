// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Exceed.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_Exceed::UWxEffect_Exceed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(6.f));

	// 중첩 불가: 대상당 1스택만 유지, 재적용 시 Duration 갱신.
	// UGameplayEffect::StackingType 은 5.7 에서 UE_DEPRECATED 처리되었으나 SetStackingType 은 WITH_EDITOR 한정이라
	// 생성자에서 사용할 수 없어 직접 할당한다. 향후 엔진이 실제로 private 화 되면 데이터 자산으로 이전 검토.
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
