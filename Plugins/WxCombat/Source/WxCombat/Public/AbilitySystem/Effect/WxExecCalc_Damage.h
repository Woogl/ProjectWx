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
 * 베이스 대미지는 SetByCaller.RawDamage가 양수면 그 값 그대로(환경 대미지), 아니면 SourceATK * Coeff.ATK * (100 / (100 + TargetDEF))다.
 *
 * 적용은 대상 상태로 갈린다.
 *  - 무적          : 대미지 무효, 회피 성공 이벤트
 *  - 퍼펙트 가드   : 공격자에게 DP 반사, 크리 없음
 *  - 일반 가드     : 감소율 적용 후 HP·DP·SP 차감
 *  - Unblockable 가드 : 가드 어빌리티를 끊고 HP·DP 차감(퍼펙트 가드도 뚫는다)
 *  - 비가드        : HP·DP 차감
 */
UCLASS()
class WXCOMBAT_API UWxExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UWxExecCalc_Damage();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
	/** 무적이면 회피 성공 이벤트를 보내고 true를 반환한다 */
	bool HandleInvincible(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC) const;

	/** 퍼펙트 가드 시 공격자에게 DP 반사 */
	void ReflectPerfectGuard(UAbilitySystemComponent* SourceASC, float ReflectAmount) const;

	/** bSkipCrit 또는 Raw 모드면 크리를 건너뛴다 */
	FWxDamageResult CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, bool bSkipCrit) const;

	/** 일반 가드는 SP를 차감하고, 공격 GE에 HitReact 태그가 실려 있을 때만 이벤트를 보낸다 */
	void ApplyHitReaction(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput, float FinalDamage) const;

	void ExecuteGameplayCueDamage(UAbilitySystemComponent* TargetASC, float DamageAmount, FVector HitLocation, const FGameplayEffectSpec& OwningSpec, bool bIsCritical) const;
	void ExecuteGameplayCuePerfectGuard(UAbilitySystemComponent* TargetASC, FVector HitLocation, const FGameplayEffectSpec& OwningSpec) const;
};
