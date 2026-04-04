// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_DodgeSuccess.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitInputPress;

/**
 * 회피 성공(극한 회피) 반응 어빌리티.
 *
 * 사용 흐름:
 *  1. 무적 구간에서 대미지 회피 → Event.DodgeSuccess 이벤트 발송 (ExecCalc)
 *  2. GameplayEvent 트리거 → ActivateAbility (ServerInitiated)
 *  3. 회피 어빌리티를 캔슬하고 DodgeSuccessMontage 재생
 *  4. ANS_ComboWindow 구간 내 공격 입력 시 DodgeCounterMontage로 전환
 *  5. 몽타주 완료/중단 → EndAbility
 *
 * 공격 입력 감지를 위해 활성화 시 Input.Attack 태그를 스펙에 동적으로 추가하고,
 * WaitInputPress 태스크로 수신한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_DodgeSuccess : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_DodgeSuccess();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DodgeSuccessMontage;

	/** 회피 성공 중 공격 입력 시 재생할 반격 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DodgeCounterMontage;

private:
	void PlayDodgeCounterMontage();

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
};
