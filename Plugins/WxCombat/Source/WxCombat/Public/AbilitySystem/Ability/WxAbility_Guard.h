// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Guard.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

/**
 * 페이즈 (ActiveMontage로 판별):
 *  GuardMontage          – State.Guard 활성, 루핑
 *  GuardHitReactMontage  – 일반 가드 피격(SP 여유) 후 GuardMontage 복귀
 *  GuardKnockbackMontage – Knock 계열 피격 가드 후 GuardMontage 복귀
 *  GuardBreakMontage     – SP 고갈로 State.Guard 해제 후 재생, 종료
 *  PerfectGuardMontage   – 퍼펙트 가드 성공 후 재생, 완주하면 종료(가드 키를 쥐고 있으면 가드가 새로 발동해 복귀)
 *                          완주 전에 후속 피격이 오면 리액션 페이즈로 끊긴다
 *
 * Unblockable 피격은 퍼펙트 가드 윈도우 중이라도 가드로 막히지 않는다.
 * ExecCalc가 이 어빌리티를 직접 Cancel한 뒤 HitReact 이벤트를 발송한다.
 *
 * 가드 반격은 여기서 다루지 않는다 — State.Guard만 발행하면 공격 어빌리티가 그 태그로 자기 반격 세트를 고른다.
 * 진입 시점은 가드 몽타주의 StartRecovery가 차단을 푸는 때다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Guard : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Guard();

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	float GetDamageReductionRate() const;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardHitReactMontage;

	/** Knockback/Knockup/Knockdown 공격을 가드했을 때 재생하는 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardKnockbackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardBreakMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> PerfectGuardMontage;

	/** 가드 중 받는 대미지 배율(0~1). 0.5면 50% 감소. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|PerfectGuard", meta = (ClampMin = "0", ClampMax = "1"))
	float DamageReductionRate = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|PerfectGuard")
	float PerfectGuardMPRecovery = 5.f;
	
	/** 퍼펙트 가드 성공 시 적용할 GlobalTimeDilation 값 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|SlowTime", meta = (ClampMin = "0.01"))
	float PerfectGuardSlowTimeDilation = 0.4f;

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
