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
 *  GuardHitReactMontage – 일반 가드 피격(SP 여유) 시 재생 후 GuardMontage 복귀
 *  GuardBreakMontage    – SP 고갈 시 State.Guard 해제 후 재생, 종료
 *
 * Unblockable 피격 처리:
 *  PerfectGuard 윈도우 중이면 ExecCalc가 PerfectGuard로 처리한다.
 *  일반 Guard 중 Unblockable 피격 시 ExecCalc가 Guard 어빌리티를 직접 Cancel하고 HitReact 이벤트를 발송한다.
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

	/** 퍼펙트 가드 성공 시 회복하는 MP량 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Recovery")
	float PerfectGuardMPRecovery = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardHitReactMontage;

	/** Knockback/Knockup/Knockdown 공격을 가드했을 때 재생하는 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardKnockbackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardBreakMontage;

private:
	bool PlayMontage(UAnimMontage* Montage);
	void ListenForGuardHit();
	void ListenForPerfectGuard();

	UFUNCTION()
	void HandleGuardHitReact(FGameplayEventData Payload);

	UFUNCTION()
	void HandlePerfectGuard(FGameplayEventData Payload);

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
