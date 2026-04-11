// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Death.generated.h"

class UAnimMontage;

/**
 * 사망 어빌리티.
 *
 * 사용 흐름:
 *  1. HP == 0 → PostGameplayEffectExecute에서 State.Dead 태그 부여
 *  2. OwnedTagPresent 트리거 → ActivateAbility
 *  3-A. HitReact 어빌리티 활성 중 → 해당 몽타주 자연 종료 대기 (최대 0.15초) → 래그돌 활성화 → EndAbility
 *  3-B. DeathMontage 유효 → DeathMontage 재생 → 완료 시 래그돌 활성화 → EndAbility
 *  3-C. DeathMontage 무효 → 즉시 래그돌 활성화 → EndAbility
 *
 * 사망 발동 시 신규 어빌리티 차단. HitReact 재생 중이면 자연 종료 후 래그돌 (타임아웃 시 강제 래그돌).
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Death : public UWxAbilityBase
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

	UFUNCTION()
	void HandleActiveMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void FinishWaitAndRagdoll();

	void EnableRagdoll();

	UPROPERTY()
	TObjectPtr<UAnimMontage> PendingWaitMontage;

	FTimerHandle WaitTimerHandle;
};
