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
 *  2. 어트리뷰트 캡처 (ATK, DEF, CritRate, CritDMG)
 *  3. 상태 판정 (퍼펙트 가드, 가드, Unblockable)
 *  4. 대미지 적용
 *     - 퍼펙트 가드: 대미지 반사, MP 회복, HitReact 이벤트 (Unblockable 포함 모든 공격 차단)
 *     - 일반 가드 피격: HP·DP·SP 차감, GuardHit 이벤트 → Guard 어빌리티가 GuardHitReact/GuardBreak 처리
 *     - Unblockable 가드 피격: HP·DP·PP 차감, Guard 어빌리티 Cancel → HitReact 이벤트
 *     - 비가드 피격: HP·DP·PP 차감, PP 소진 시 HitReact 이벤트
 *  5. AI 대미지 감지
 *  6. 대미지 GameplayCue
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

	/** 피격 반응 적용. 가드 상태와 Damage.Knock* 태그에 따라 SP/PP 차감 및 이벤트 발송 */
	void ApplyHitReaction(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& OwningSpec, float FinalDamage, float TargetPP, bool bIsGuarding, bool bIsUnblockable, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const;

	/** 대미지 GameplayCue 실행 */
	void ExecuteGameplayCueDamage(UAbilitySystemComponent* TargetASC, float DamageAmount, FVector HitLocation, const FGameplayEffectSpec& OwningSpec, bool bIsCritical, bool bDisplayDamageFloater) const;
};
