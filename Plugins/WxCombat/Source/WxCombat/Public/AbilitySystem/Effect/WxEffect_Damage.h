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

/** SP·GP·IncomingDamage 순으로 반영한다. 전투 반응은 모든 출력 반영 후 DamageResponse 컴포넌트가 처리한다. */
UCLASS()
class WXCOMBAT_API UWxExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UWxExecCalc_Damage();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
