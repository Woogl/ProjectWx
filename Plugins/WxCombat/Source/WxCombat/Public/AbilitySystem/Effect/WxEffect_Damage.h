// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "WxEffect_Damage.generated.h"

class UAbilitySystemComponent;

UCLASS()
class WXCOMBAT_API UWxEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Damage();
};

UENUM()
enum class EWxDamageCheck : uint8
{
	None,
	Evaded,
	Damaged
};

struct FWxDamageResult
{
	float FinalDamage = 0.f;
	bool bIsCritical = false;
};

/**
 * 적중 성립(팀·시체·무적)은 CheckDamage가 적용 앞에서 가르고, 적용된 스펙은 대상 상태로 갈린다.
 *  - 퍼펙트 가드   : 반사량을 IncomingReflect로 내보내고 종료, 크리 없음
 *  - 일반 가드     : 감소율 적용 후 HP·SP 차감, DP 가산
 *  - 가드 불가     : 가드를 무시하고 HP 차감, DP 가산(퍼펙트 가드도 뚫는다)
 *  - 비가드        : HP 차감, DP 가산
 *
 * 계산해서 출력 모디파이어·스펙 태그·FWxCombatEffectContext에 싣는 것이 전부다 — 어트리뷰트 직접 기록도, GE 적용도, 어빌리티 취소도 하지 않는다.
 * 그렇게 나간 부수효과는 GE가 거부되거나 예측 롤백돼도 되돌아가지 않는다.
 * 결과를 실제 상태 변화와 연출로 옮기는 일은 UWxCombatAttributeSet::PostGameplayEffectExecute가 맡는다.
 * 어트리뷰트가 확정되기 전에 발행하면 수신자가 "차감 전 값"을 역산해야 하기 때문이다.
 * 출력 모디파이어가 하나씩 적용되며 그때마다 그 훅이 불리므로, IncomingDamage를 맨 뒤에 둬 DP가 먼저 확정되게 한다.
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
	 * ExecCalc를 건너뛰는 클라이언트 예측 경로가 서버와 같은 결론에 이르는 통로이며, 히트스톱과 투사체 임팩트 연출도 이 선판정을 그대로 쓴다.
	 *
	 * @return	사망·팀에서 걸리면 None, 무적이면 Evaded, 성립하면 Damaged. 성립이라도 대미지 값은 아직 산출 전이다.
	 */
	static EWxDamageCheck CheckDamage(const UAbilitySystemComponent* Source, const UAbilitySystemComponent* Target);

private:
	FWxDamageResult CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, bool bSkipCrit) const;
};
