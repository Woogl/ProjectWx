// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_Guard.generated.h"

class UAnimMontage;

/**
 * 가드 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 홀드 → ActivateAbility → GuardMontage 재생
 *  2. 몽타주의 ANS_Guard 구간 동안 가드 판정 활성
 *  3. 입력 릴리즈 → InputReleased → EndAbility → 몽타주 중단
 *
 * 가드 판정은 몽타주에 배치된 ANS_Guard가 ANS.Guard 태그로 제어.
 * State.Dead 시 활성화 차단.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Guard : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_Guard();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardMontage;

private:
	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
};
