// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "WxDamageExecCalc.generated.h"

/**
 * 데미지 계산 ExecutionCalculation.
 *
 * 공식: FinalDamage = SourceATK * (190 / (190 + TargetDEF))
 * 결과를 대상의 IncomingDamage 메타 어트리뷰트에 전달.
 */
UCLASS()
class WXCOMBAT_API UWxDamageExecCalc : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UWxDamageExecCalc();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
