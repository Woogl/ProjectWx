// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ability/WxGameplayAbility.h"
#include "WxGA_MeleeAttack.generated.h"

class UAnimMontage;

/**
 * 근접 공격 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 → ActivateAbility → PlayMontageAndWait
 *  2. 몽타주 내 WxANS_WeaponCollision 노티파이 스테이트가 무기 콜리전 on/off
 *  3. 몽타주 완료/중단 → EndAbility
 *
 * 무기 콜리전 안전 해제는 AWxWeaponBase가 OnMontageEnded 콜백으로 자체 관리.
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
