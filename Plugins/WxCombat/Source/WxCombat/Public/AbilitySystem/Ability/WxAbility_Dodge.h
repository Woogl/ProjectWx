// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_Dodge.generated.h"

class UAnimMontage;

/**
 * 회피 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 → ActivateAbility → DodgeMontage 재생
 *  2. 몽타주의 ANS_Invincible 구간 동안 무적
 *  3. 몽타주 완료/중단 → EndAbility
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Dodge : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DodgeMontage;

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
