// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/EngineTypes.h"
#include "WxAbility_Groggy.generated.h"

class UAnimMontage;

/**
 * DP가 MaxDP에 도달하면 붙는 State.Groggy 태그로 발동하고, 폴링으로 빈 몽타주 슬롯에 그로기 몽타주를 밀어 넣으며 DP를 드레인한다.
 * DP가 0이 되어 태그가 떨어지면 종료한다.
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
	// ActivationBlockedTags는 신규 활성화만 막고 실행 중 인스턴스는 끝내지 못하므로 별도 구독한다.
	void HandleDeadTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void HandleMontagePollTick();

	// 실패복구: DrainDP가 DP를 0까지 못 내려 그로기가 끝나지 않으면 DP를 강제 리셋해 종료시킨다.
	void HandleGroggySafetyTimeout();

	FDelegateHandle DeadTagDelegateHandle;
	FActiveGameplayEffectHandle DrainDPEffectHandle;
	FTimerHandle MontagePollingTimerHandle;
	FTimerHandle GroggySafetyTimerHandle;
};
