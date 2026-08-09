// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Sprint.generated.h"

/**
 * 입력을 누르는 동안 SPD 배율 GameplayEffect를 걸어 두고, 떼면 걷어낸다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Sprint : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Sprint();

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** SPD에 곱해지는 이동 속도 배율(1.0 = 평상시 속도) */
	UPROPERTY(EditDefaultsOnly)
	float SprintSpeedScale = 1.5f;

	FActiveGameplayEffectHandle SpeedEffectHandle;
};
