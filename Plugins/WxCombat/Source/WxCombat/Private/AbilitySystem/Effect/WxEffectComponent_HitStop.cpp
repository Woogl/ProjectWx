// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComponent_HitStop.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_HitStop.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Weapon/WxProjectileBase.h"
#include "Weapon/WxWeaponBase.h"
#include "WxGameplayTags.h"

void UWxEffectComponent_HitStop::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
	UAbilitySystemComponent* Target = ActiveGEContainer.Owner;
	UAbilitySystemComponent* Source = GESpec.GetContext().GetInstigatorAbilitySystemComponent();

	// Hit Cue와 같은 조건이다 — 타격 연출 없이 경직만 남지 않게 한다.
	const FGameplayEffectModifiedAttribute* DamageRecord = GESpec.GetModifiedAttribute(UWxCombatAttributeSet::GetIncomingDamageAttribute());
	const float Damage = DamageRecord ? DamageRecord->TotalMagnitude : 0.f;
	if (Damage <= 0.f && !GESpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_PerfectGuarded))
	{
		return;
	}

	// 반응 뒤에 걸어야 공격자는 동기로 도착한 반응(패리 등)에 몽타주를 양보하고, 피격자는 막 시작된 반응 몽타주를 얼린다.
	const AActor* Causer = GESpec.GetContext().GetEffectCauser();
	if (const AWxWeaponBase* Weapon = Cast<AWxWeaponBase>(Causer))
	{
		UWxEffect_HitStop::Apply(Weapon->InstigatorHitStop, Source, Source);
		UWxEffect_HitStop::Apply(Weapon->VictimHitStop, Source, Target);
	}
	else if (const AWxProjectileBase* Projectile = Cast<AWxProjectileBase>(Causer))
	{
		UWxEffect_HitStop::Apply(Projectile->InstigatorHitStop, Source, Source);
		UWxEffect_HitStop::Apply(Projectile->VictimHitStop, Source, Target);
	}
}
