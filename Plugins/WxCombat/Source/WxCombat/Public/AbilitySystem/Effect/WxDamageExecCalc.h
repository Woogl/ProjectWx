// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "WxDamageExecCalc.generated.h"

class UAbilitySystemComponent;

struct FWxDamageResult
{
	float FinalDamage = 0.f;
	bool bIsCritical = false;
};

/**
 * 데미지 계산 ExecutionCalculation.
 *
 * 공식: FinalDamage = SourceATK * (100 / (100 + TargetDEF))
 * 결과를 대상의 IncomingDamage 메타 어트리뷰트에 전달.
 */
UCLASS()
class WXCOMBAT_API UWxDamageExecCalc : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UWxDamageExecCalc();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
	bool HandlePerfectGuard(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, AActor* SourceActor, AActor* TargetActor, float SourceATK, float DefenseMultiplier) const;

	FWxDamageResult CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, float SourceATK, float DefenseMultiplier, UAbilitySystemComponent* TargetASC) const;

	void ApplyPostDamageEffects(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, AActor* SourceActor, AActor* TargetActor, const FGameplayEffectSpec& OwningSpec, const FWxDamageResult& DamageResult) const;
};
