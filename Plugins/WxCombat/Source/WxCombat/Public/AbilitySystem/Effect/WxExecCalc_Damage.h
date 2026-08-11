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
 *  - 무적          : 대미지 무효
 *  - 퍼펙트 가드   : 공격자에게 DP 반사, 크리 없음
 *  - 일반 가드     : 감소율 적용 후 HP·DP·SP 차감
 *  - Unblockable 가드 : 가드 어빌리티를 끊고 HP·DP 차감(퍼펙트 가드도 뚫는다)
 *  - 비가드        : HP·DP 차감
 *
 * 여기서는 GameplayCue도 GameplayEvent도 발행하지 않는다.
 * 판정 결과를 FWxCombatEffectContext에 남기면, GE 적용이 끝난 뒤 UWxAbilitySystemComponent::HandleGameplayEffectAppliedToSelf가 그걸 읽어 발행한다.
 * 어트리뷰트가 확정되기 전에 발행하면 수신자가 "차감 전 값"을 역산해야 하기 때문이다.
 */
UCLASS()
class WXCOMBAT_API UWxExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UWxExecCalc_Damage();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
	void ReflectPerfectGuard(UAbilitySystemComponent* SourceASC, float ReflectAmount) const;

	/** bSkipCrit 또는 Raw 모드면 크리를 건너뛴다 */
	FWxDamageResult CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, bool bSkipCrit) const;

	/** 가드 반응(일반 가드의 SP 차감, Unblockable의 가드 해제)을 적용하고 피격자에게 보낼 반응 이벤트 태그를 정한다 */
	FGameplayTag ResolveHitReaction(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput, float FinalDamage) const;
};
