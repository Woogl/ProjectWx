// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_Sprint.generated.h"

/**
 * 스프린트 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 홀드 → ActivateAbility → SPD +0.5 GameplayEffect 적용
 *  2. 입력 릴리즈 → InputReleased → EndAbility → Effect 제거
 *
 * State.Dead 시 활성화 차단.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Sprint : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_Sprint();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float SprintSpeedBonus = 1.f;

private:
	UPROPERTY()
	TObjectPtr<UGameplayEffect> SprintSpeedEffect;

	FActiveGameplayEffectHandle SpeedEffectHandle;
};
