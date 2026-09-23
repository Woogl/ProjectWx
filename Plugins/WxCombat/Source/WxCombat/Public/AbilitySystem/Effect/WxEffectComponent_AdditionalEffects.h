// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "WxEffectComponent_AdditionalEffects.generated.h"

/** 피해 행의 추가 효과를 반응이 끝난 뒤 대상에게 적용한다. 퍼펙트 가드면 적용하지 않는다. */
UCLASS()
class WXCOMBAT_API UWxEffectComponent_AdditionalEffects : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
};
