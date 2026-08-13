// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "WxExecCalc_Burn.generated.h"

/**
 * 화상 데미지 ExecutionCalculation.
 * 가드·퍼펙트 가드·치명타 판정 없이 공식 대미지를 NumTicks 로 균등 분할해 적용한다.
 *
 * 대미지 ExecCalc과 같이 여기서는 Cue를 발행하지 않는다.
 * 틱 결과를 FWxCombatEffectContext에 남기면 UWxAbilitySystemComponent::HandlePeriodicGameplayEffectExecuted가 읽어 발행한다.
 */
UCLASS()
class WXCOMBAT_API UWxExecCalc_Burn : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UWxExecCalc_Burn();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	static constexpr float BurnDuration = 5.f;
	static constexpr float BurnPeriod = 0.5f;
	static constexpr int32 NumTicks = static_cast<int32>(BurnDuration / BurnPeriod);
};
