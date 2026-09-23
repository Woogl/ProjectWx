// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "WxEffectComponent_PerfectGuard.generated.h"

/** 퍼펙트 가드로 막은 피해 GE에서 가드 이벤트·반사 GP·패리·Cue와 투사체 되돌림을 낸다. */
UCLASS()
class WXCOMBAT_API UWxEffectComponent_PerfectGuard : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
};
