// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/EngineTypes.h"
#include "WxAbility_Groggy.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * GP가 MaxGP에 닿으면 AttributeSet이 송출하는 Event.Groggy로 발동해 빈 몽타주 슬롯에 그로기 몽타주를 재생하고 GP를 드레인한다.
 * 서버만 드레인과 종료 판정을 하고, 소유 클라는 자세 몽타주만 재생한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Groggy : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Groggy();

protected:
	virtual bool PlayMontageInternal(UAnimMontage* Montage, FName StartSection) override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void HandleGPChanged(const FOnAttributeChangeData& Data);

	void HandleMontagePollTick();
	void StartMontagePolling();
	void StopMontagePolling();

	void StartGroggyDrain(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);
	void StopGroggyDrain(UAbilitySystemComponent& ASC);

	FDelegateHandle GPDelegateHandle;
	FActiveGameplayEffectHandle DrainGPEffectHandle;
	FTimerHandle MontagePollingTimerHandle;
};
