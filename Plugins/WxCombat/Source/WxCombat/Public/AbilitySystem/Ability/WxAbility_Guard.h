// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_Guard.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

/**
 * 가드 어빌리티.
 *
 * 페이즈 (ActiveMontage로 판별):
 *  GuardMontage         – State.Guard 태그 활성, 피격 시 HitReact 전환
 *  GuardHitReactMontage – 재생 후 GuardMontage 복귀
 *  GuardBreakMontage    – SP 고갈 시 State.Guard 해제 후 재생, 종료
 *
 * 입력 릴리즈 시 GuardBreak 중이 아니면 즉시 EndAbility.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Guard : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_Guard();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardBreakMontage;

private:
	bool PlayMontage(UAnimMontage* Montage);
	void ListenForHitReact();

	UFUNCTION()
	void HandleGuardHitReact(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageBlendingOut();

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;
};
