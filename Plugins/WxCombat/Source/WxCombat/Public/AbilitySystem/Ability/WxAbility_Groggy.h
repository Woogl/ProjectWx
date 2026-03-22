// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_Groggy.generated.h"

class UAnimMontage;

/**
 * 그로기 어빌리티.
 *
 * 사용 흐름:
 *  1. DP가 MaxDP에 도달 → State.Groggy 태그 부여 + Event.Groggy 이벤트 발송
 *  2. GameplayEvent 트리거 → ActivateAbility
 *  3. 몽타주 재생, State.Groggy 태그 제거 감지 시 몽타주 중단 → EndAbility
 *
 * 그로기 발동 중 HitReact를 제외한 모든 어빌리티 차단.
 * State.Dead 시 활성화 차단.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Groggy : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_Groggy();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GroggyMontage;

private:
	void HandleGroggyTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	void PlayGroggyMontage();
	
	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	FDelegateHandle GroggyTagDelegateHandle;
};
