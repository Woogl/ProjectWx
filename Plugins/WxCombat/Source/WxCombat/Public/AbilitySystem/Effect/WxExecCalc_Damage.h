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
 *  1. 무적 판정 → 대미지 무효, 회피 성공 보상
 *  2. 베이스 대미지 계산
 *     - SetByCaller.RawDamage 양수: ATK/DEF/Coeff 우회, RawDamage 값을 그대로 사용 (환경 대미지)
 *     - 그 외: SourceATK * Coeff.ATK * (100 / (100 + TargetDEF))
 *  3. 상태 판정 (퍼펙트 가드, 가드, Unblockable)
 *  4. 대미지 적용
 *     - 퍼펙트 가드: 대미지 반사, MP 회복, HitReact 이벤트 (Unblockable 공격은 퍼펙트 가드 불가)
 *     - 일반 가드 피격: HP·DP·SP 차감, GuardHit 이벤트 → Guard 어빌리티가 GuardHitReact/GuardBreak 처리
 *     - Unblockable 가드 피격: HP·DP·PP 차감, Guard 어빌리티 Cancel → HitReact 이벤트
 *     - 비가드 피격: HP·DP·PP 차감, PP 소진 시 HitReact 이벤트
 *     - 크리는 Raw 모드에서 적용되지 않음
 *  5. AI 대미지 감지
 *  6. 대미지 GameplayCue
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

	/** 베이스 대미지(Raw 또는 ATK·DEF 공식) 계산 후 크리 적용. bSkipCrit 또는 Raw 모드면 크리 스킵 */
	FWxDamageResult CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, bool bSkipCrit) const;

	/** 공격자 적중 회복. OwningSpec의 SetByCaller.Recovery.UP/MP 값을 단일 Recovery Spec에 실어 공격자에게 적용 */
	void ApplyHitRecovery(UAbilitySystemComponent* SourceASC, const FGameplayEffectSpec& OwningSpec) const;

	/** 피격 반응 적용. 가드 상태와 Damage.Knock* 태그에 따라 SP/PP 차감 및 이벤트 발송 */
	void ApplyHitReaction(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput, float FinalDamage) const;

	/** 대미지 GameplayCue 실행 */
	void ExecuteGameplayCueDamage(UAbilitySystemComponent* TargetASC, float DamageAmount, FVector HitLocation, const FGameplayEffectSpec& OwningSpec, bool bIsCritical) const;

	/** 퍼펙트 가드 GameplayCue 실행 */
	void ExecuteGameplayCuePerfectGuard(UAbilitySystemComponent* TargetASC, FVector HitLocation, const FGameplayEffectSpec& OwningSpec) const;
};
