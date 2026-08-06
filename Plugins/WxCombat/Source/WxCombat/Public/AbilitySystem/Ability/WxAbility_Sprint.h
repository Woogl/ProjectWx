// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Sprint.generated.h"

/**
 * 스프린트 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 홀드 → ActivateAbility → SPD 배율 GameplayEffect 적용
 *  2. 입력 릴리즈 → InputReleased → EndAbility → Effect 제거
 *
 * State.Dead 시 활성화 차단.
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
	/** 스프린트 중 이동 속도 배율. SPD 에 곱해진다. (1.0 = 평상시 속도) */
	UPROPERTY(EditDefaultsOnly)
	float SprintSpeedScale = 1.5f;

	FActiveGameplayEffectHandle SpeedEffectHandle;
};
