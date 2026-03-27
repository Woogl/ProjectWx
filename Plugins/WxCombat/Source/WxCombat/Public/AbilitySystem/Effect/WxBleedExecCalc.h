// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "WxBleedExecCalc.generated.h"

/**
 * 출혈 데미지 ExecutionCalculation.
 *
 * 공식: TickDamage = (SourceATK × (100 / (100 + TargetDEF))) / NumTicks
 * 가드, 퍼펙트 가드, 치명타 판정 없이 순수 공식 대미지를 균등 분할하여 적용한다.
 * 결과를 대상의 IncomingDamage와 DP에 전달한다.
 */
UCLASS()
class WXCOMBAT_API UWxBleedExecCalc : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UWxBleedExecCalc();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	static constexpr float BleedDuration = 5.f;
	static constexpr float BleedPeriod = 0.5f;
	static constexpr int32 NumTicks = static_cast<int32>(BleedDuration / BleedPeriod);
};
