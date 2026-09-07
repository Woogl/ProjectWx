// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "WxEffectComponent_DamageResponse.generated.h"

class UAbilitySystemComponent;

/** 피해 GE의 속성 반영 후 전투 이벤트와 플로터를 실행한다. 실행별 상태는 GE Spec에서만 읽는다. */
UCLASS()
class WXCOMBAT_API UWxEffectComponent_DamageResponse : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

private:
	void ProcessDamageTaken(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float Damage) const;
	void ProcessPerfectGuard(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, float ReflectAmount) const;
};
