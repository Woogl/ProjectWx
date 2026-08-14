// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Damage/WxCombatEffectContext.h"
#include "WxExecCalc_Damage.generated.h"

class UAbilitySystemComponent;

struct FWxDamageResult
{
	float FinalDamage = 0.f;
	bool bIsCritical = false;
};

/**
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

	/**
	 * 대미지 계산에 앞서 적중 자체가 성립하는지 가른다.
	 *
	 * 어트리뷰트를 보지 않아 ExecCalc 밖에서도 돌릴 수 있다.
	 * ExecCalc를 건너뛰는 클라이언트 예측 경로가 서버와 같은 결론에 이르는 통로이며, 화상과 히트스톱도 이 선판정을 그대로 쓴다.
	 *
	 * @return	사망·팀에서 걸리면 None, 무적이면 Evaded, 성립하면 Damaged. 성립이라도 대미지 값은 아직 산출 전이다.
	 */
	static EWxDamageResult CheckDamage(const UAbilitySystemComponent* Source, const UAbilitySystemComponent* Target);

private:
	void ReflectPerfectGuard(UAbilitySystemComponent* SourceASC, float ReflectAmount) const;

	/** Raw 모드면 bSkipCrit과 무관하게 크리를 건너뛴다 */
	FWxDamageResult CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, bool bSkipCrit) const;

	/** 가드 반응(일반 가드의 SP 차감, Unblockable의 가드 해제)을 적용하고 피격자에게 보낼 반응 이벤트 태그를 정한다 */
	FGameplayTag ResolveHitReaction(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput, float FinalDamage) const;
};
