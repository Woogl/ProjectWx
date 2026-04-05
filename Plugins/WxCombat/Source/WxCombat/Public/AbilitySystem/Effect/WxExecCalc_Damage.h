// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "WxExecCalc_Damage.generated.h"

class UAbilitySystemComponent;

struct FWxDamageResult
{
	float FinalDamage = 0.f;
	bool bIsCritical = false;
};

/**
 * 데미지 계산 ExecutionCalculation.
 *
 * 판정 흐름:
 *  1. 무적 판정 (극한 회피) → 대미지 무효, 회피 성공 보상
 *  2. 퍼펙트 가드 판정 → 대미지 무효, DP 반사, 방어자 MP 회복
 *  3. 일반/가드 판정 → 대미지 계산, 가드 감소 (Unblockable이면 무시), 공격자 자원 회복
 *  4. PP 차감 → HitReact 발동, AI 감지, 대미지 Cue
 *
 * 대미지 공식: FinalDamage = SourceATK * (100 / (100 + TargetDEF))
 */
UCLASS()
class WXCOMBAT_API UWxExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UWxExecCalc_Damage();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
	/** 무적 상태(극한 회피) 처리. 회피 성공 시 MP 회복 및 이벤트 발송. 무적이면 true 반환 */
	bool HandleInvincible(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC) const;

	/** 퍼펙트 가드 시 공격자에게 DP 반사 */
	void ReflectPerfectGuard(UAbilitySystemComponent* SourceASC, float ReflectAmount) const;

	/** 대미지 계산. ATK·DEF 공식, 치명타, 가드 감소 적용 */
	FWxDamageResult CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, float SourceATK, float DefenseMultiplier, UAbilitySystemComponent* TargetASC, bool bIsUnblockable) const;

	/** 공격 적중 시 공격자 자원 회복. 평타 → MP, 스킬 → UP */
	void RecoverAttackerResource(UAbilitySystemComponent* SourceASC, const FGameplayEffectSpec& OwningSpec) const;

	void ApplyResourceRecovery(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> RecoveryEffect, float Amount) const;

	void ExecuteGameplayCueDamage(UAbilitySystemComponent* TargetASC, float DamageAmount, FVector HitLocation, const FGameplayEffectSpec& OwningSpec, bool bIsCritical, bool bDisplayDamageFloater) const;
};
