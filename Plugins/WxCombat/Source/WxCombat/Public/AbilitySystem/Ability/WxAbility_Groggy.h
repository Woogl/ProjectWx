// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/EngineTypes.h"
#include "WxAbility_Groggy.generated.h"

class UAnimMontage;

/**
 * 그로기 어빌리티.
 *
 * 사용 흐름:
 *  1. DP가 MaxDP에 도달 → State.Groggy 태그 부여
 *  2. OwnedTagPresent 트리거로 ActivateAbility
 *  3. 폴링으로 활성 몽타주가 없을 때 그로기 몽타주 재생, DP 드레인 적용
 *  4. State.Groggy 태그 제거 감지 시 EndAbility
 *
 * 그로기 발동 중 HitReact를 제외한 모든 어빌리티 차단.
 * State.Dead 시 활성화 차단.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Groggy : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Groggy();

	/** 그로기 중 받는 대미지 배율(>=1). ExecCalc 가 그로기 피격 시 이 값을 곱한다. */
	float GetDamageTakenMultiplier() const;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GroggyMontage;

	/** 그로기 중 받는 대미지 배율(>=1). 1.3 이면 30% 증가. ExecCalc_Damage 가 그로기 피격 분기에서 참조. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability", meta = (ClampMin = "1"))
	float DamageTakenMultiplier = 1.3f;

private:
	void HandleGroggyTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void TickPlayMontage();

	FDelegateHandle GroggyTagDelegateHandle;
	FActiveGameplayEffectHandle DrainDPEffectHandle;
	FTimerHandle MontagePollingTimerHandle;
};
