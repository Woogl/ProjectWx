// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "WxEffect_Damage.generated.h"

UCLASS()
class WXCOMBAT_API UWxEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Damage();
};

/** 방어를 판정해 계산하고 SP·IncomingDamage·GP 순으로 반영한다. 반응은 Damage GE의 컴포넌트들이 처리한다. */
UCLASS()
class WXCOMBAT_API UWxExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UWxExecCalc_Damage();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
