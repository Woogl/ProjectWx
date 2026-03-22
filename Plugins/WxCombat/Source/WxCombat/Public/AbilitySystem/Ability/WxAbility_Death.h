// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_Death.generated.h"

class UAnimMontage;

/**
 * 사망 어빌리티.
 *
 * 사용 흐름:
 *  1. HP == 0 → PostGameplayEffectExecute에서 State.Dead 태그 부여
 *  2. OwnedTagPresent 트리거 → ActivateAbility
 *  3-A. DeathMontage 유효 → 몽타주 재생 → 완료 시 래그돌 활성화 → EndAbility
 *  3-B. DeathMontage 무효 → 즉시 래그돌 활성화 → EndAbility
 *
 * 사망 발동 시 HitReact를 포함한 모든 어빌리티 캔슬 및 차단.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Death : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_Death();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DeathMontage;

private:
	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	void EnableRagdoll();
};
