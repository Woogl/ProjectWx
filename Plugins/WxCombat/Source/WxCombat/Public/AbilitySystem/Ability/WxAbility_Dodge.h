// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Dodge.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitInputPress;
struct FGameplayAbilityTargetDataHandle;

/**
 * 회피 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 → ActivateAbility → DodgeMontage 재생, Event.DodgeSuccess 대기
 *  2. 몽타주의 State.Invincible 구간 동안 무적
 *  3. 무적 중 피격(극한 회피) → PerfectDodgeMontage 재생
 *  4. ANS_ComboWindow 구간 내 공격 입력 시 DodgeCounterMontage로 전환
 *  5. 몽타주 완료/중단 → EndAbility
 *
 * 클라이언트는 입력 방향을 TargetData로 서버에 전송하여
 * 서버에서도 올바른 방향으로 회전.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Dodge : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DodgeMontage;

	/** 극한 회피 성공 시 재생할 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> PerfectDodgeMontage;

	/** 극한 회피 성공 중 공격 입력 시 재생할 반격 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DodgeCounterMontage;

	/** 극한 회피 성공 시 회복하는 MP량 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	float PerfectDodgeMPRecovery = 5.f;

private:
	void ApplyDodgeDirection(const FVector& Direction);
	void HandleTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);
	void ListenForDodgeSuccess();
	void PlayPerfectDodgeMontage();
	void PlayDodgeCounterMontage();
	void ListenForCounterInput();

	UFUNCTION()
	void HandleDodgeSuccess(FGameplayEventData Payload);

	UFUNCTION()
	void HandleCounterInputPressed(float TimeWaited);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputPress> WaitInputTask;

	/** 극한 회피 상태 (PerfectDodge/Counter 몽타주 재생 중) */
	bool bPlayingPerfectDodge = false;
};
