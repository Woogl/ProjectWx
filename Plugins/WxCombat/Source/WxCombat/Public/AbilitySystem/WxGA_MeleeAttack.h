// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/WxGameplayAbility.h"
#include "WxGA_MeleeAttack.generated.h"

class UAnimMontage;

/**
 * 근접 공격 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 → ActivateAbility → AttackMontage 재생
 *  2. 몽타주 완료/중단 → EndAbility
 *
 * 콤보 체인은 GAS 태그 제약(ActivationRequiredTags, ActivationBlockedTags)으로 구성.
 * ANS_ComboWindow 구간에서 공격 입력 시 GAS가 태그 조건에 맞는 다음 콤보를 자동 활성화.
 */
UCLASS()
class WXCOMBAT_API UWxGA_MeleeAttack : public UWxGameplayAbility
{
	GENERATED_BODY()

public:
	UWxGA_MeleeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> AttackMontage;

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
