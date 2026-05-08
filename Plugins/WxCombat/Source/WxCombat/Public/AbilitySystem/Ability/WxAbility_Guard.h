// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Guard.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

/**
 * 가드 어빌리티.
 *
 * 페이즈 (ActiveMontage로 판별):
 *  GuardMontage         – State.Guard 태그 활성, 피격 시 HitReact 전환 (루핑 몽타주)
 *  GuardHitReactMontage – 일반 가드 피격(SP 여유) 시 재생 후 GuardMontage 복귀
 *  GuardKnockbackMontage – Knockback 계열 피격 가드 시 재생 후 GuardMontage 복귀
 *  GuardBreakMontage    – SP 고갈 시 State.Guard 해제 후 재생, 종료
 *
 * Unblockable 피격 처리:
 *  PerfectGuard 윈도우 중이면 ExecCalc가 PerfectGuard로 처리한다.
 *  일반 Guard 중 Unblockable 피격 시 ExecCalc가 Guard 어빌리티를 직접 Cancel하고 HitReact 이벤트를 발송한다.
 *
 * 입력 릴리즈 시 GuardBreak 중이 아니면 즉시 EndAbility.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Guard : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Guard();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	/** 가드 중 받는 대미지 배율(0~1). ExecCalc 가 가드 피격 시 이 값을 곱한다. */
	float GetDamageReductionRate() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardHitReactMontage;

	/** Knockback/Knockup/Knockdown 공격을 가드했을 때 재생하는 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardKnockbackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardBreakMontage;

	/** 가드 중 받는 대미지 배율(0~1). 0.5 면 50% 감소. ExecCalc_Damage 가 가드 피격 분기에서 참조. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability", meta = (ClampMin = "0", ClampMax = "1"))
	float DamageReductionRate = 0.5f;

	/** 퍼펙트 가드 성공 시 회복하는 MP량 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Recovery")
	float PerfectGuardMPRecovery = 5.f;
	
	/** 퍼펙트 가드 성공 시 적용할 GlobalTimeDilation 값 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|SlowTime", meta = (ClampMin = "0.01"))
	float PerfectGuardSlowTimeDilation = 0.4f;

	/** 퍼펙트 가드 성공 시 슬로우 타임 지속 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|SlowTime", meta = (ClampMin = "0.0"))
	float PerfectGuardSlowTimeDuration = 0.4f;

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
