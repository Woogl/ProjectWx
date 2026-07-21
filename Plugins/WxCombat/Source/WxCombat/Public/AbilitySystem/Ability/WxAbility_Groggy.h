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

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GroggyMontage;

private:
	void HandleGroggyTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	// 그로기 도중 사망 시 즉시 종료. ActivationBlockedTags(State.Dead)는 신규 활성화만 막고 실행 중 인스턴스는 종료하지 못하므로 별도 구독한다.
	void HandleDeadTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void TickPlayMontage();

	// 실패복구: DrainDP가 DP를 0까지 못 내려 그로기가 끝나지 않을 때, 지속시간을 넘기면 DP를 강제 리셋해 종료시킨다.
	void HandleGroggySafetyTimeout();

	FDelegateHandle GroggyTagDelegateHandle;
	FDelegateHandle DeadTagDelegateHandle;
	FActiveGameplayEffectHandle DrainDPEffectHandle;
	FTimerHandle MontagePollingTimerHandle;
	FTimerHandle GroggySafetyTimerHandle;
};
