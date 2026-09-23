// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "WxEffectComponent_DamageReaction.generated.h"

class UAbilitySystemComponent;

/** 피해 GE의 실행 기록으로 플로터·타격 Cue·피격·가해 반응을 낸다. */
UCLASS()
class WXCOMBAT_API UWxEffectComponent_DamageReaction : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

private:
	void ProcessDamageTaken(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float Damage) const;
};
