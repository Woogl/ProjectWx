// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "WxEffectComponent_Hit.generated.h"

class UAbilitySystemComponent;

/** 적용 조건은 무부수효과로 검사하고, 회피·방어 판정과 피해 후 반응은 적용 훅에서 처리한다. */
UCLASS()
class WXCOMBAT_API UWxEffectComponent_Hit : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual bool CanGameplayEffectApply(const FActiveGameplayEffectsContainer& ActiveGEContainer, const FGameplayEffectSpec& GESpec) const override;
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

private:
	void ProcessDamageTaken(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float Damage) const;
	void ProcessPerfectGuard(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float ReflectAmount) const;
};
