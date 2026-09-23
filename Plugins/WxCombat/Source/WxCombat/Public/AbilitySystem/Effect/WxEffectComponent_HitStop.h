// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "WxEffectComponent_HitStop.generated.h"

/** 피해가 들어가거나 퍼펙트 가드가 나면 원인 액터(무기·투사체)의 설정대로 공격자·피격자에게 히트스톱을 건다. */
UCLASS()
class WXCOMBAT_API UWxEffectComponent_HitStop : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
};
