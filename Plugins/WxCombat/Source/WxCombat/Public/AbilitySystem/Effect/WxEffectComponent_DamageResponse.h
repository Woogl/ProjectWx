// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "WxEffectComponent_DamageResponse.generated.h"

/** 피해 GE의 실행 결과를 Context에 기록하고 플로터를 출력한다. 전투 반응은 Hit Wrapper가 담당한다. */
UCLASS()
class WXCOMBAT_API UWxEffectComponent_DamageResponse : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
};
