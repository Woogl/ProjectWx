// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComponent_DamageResponse.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "Damage/WxHitEffectContext.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "WxGameplayTags.h"

void UWxEffectComponent_DamageResponse::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
	UAbilitySystemComponent* ASC = ActiveGEContainer.Owner;
	if (!ASC)
	{
		return;
	}

	// 메타 속성은 이미 초기화됐다. 실행 기록을 사용해 남은 HP로 플로터 수치가 잘리지 않게 한다.
	const FGameplayEffectModifiedAttribute* Damage = GESpec.GetModifiedAttribute(UWxCombatAttributeSet::GetIncomingDamageAttribute());
	if (FWxHitEffectContext* Context = FWxHitEffectContext::Get(GESpec.GetContext()))
	{
		const FGameplayEffectModifiedAttribute* Reflect = GESpec.GetModifiedAttribute(UWxCombatAttributeSet::GetIncomingReflectAttribute());
		Context->DamageMagnitude = Damage ? Damage->TotalMagnitude : 0.f;
		Context->ReflectMagnitude = Reflect ? Reflect->TotalMagnitude : 0.f;
		Context->bHasReflect = Reflect != nullptr;
		Context->DamageResultTags = GESpec.GetDynamicAssetTags();
		Context->DamageTargetTags = *GESpec.CapturedTargetTags.GetAggregatedTags();
	}
	if (Damage && Damage->TotalMagnitude > 0.f && GESpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Attack))
	{
		FGameplayCueParameters CueParams;
		CueParams.EffectContext = GESpec.GetContext();
		CueParams.RawMagnitude = Damage->TotalMagnitude;
		CueParams.AggregatedSourceTags = GESpec.GetDynamicAssetTags();
		ASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_DamageFloater, CueParams);
	}
}
