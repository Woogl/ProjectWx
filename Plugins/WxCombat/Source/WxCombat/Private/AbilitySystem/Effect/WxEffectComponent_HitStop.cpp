// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComponent_HitStop.h"
#include "AbilitySystem/Effect/WxEffect_HitStop.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Weapon/WxProjectileBase.h"
#include "Weapon/WxWeaponBase.h"

void UWxEffectComponent_HitStop::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
	UAbilitySystemComponent* Target = ActiveGEContainer.Owner;
	UAbilitySystemComponent* Source = GESpec.GetContext().GetInstigatorAbilitySystemComponent();

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
