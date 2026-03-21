// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_Skill.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * 스킬 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 → ActivateAbility → 첫 번째 콤보 몽타주 재생
 *  2. 콤보 윈도우 중 재입력 → InputPressed → 다음 콤보 몽타주로 전환
 *  3. 마지막 몽타주 완료 또는 콤보 미입력 → EndAbility
 *
 * 콤보 체인은 ComboMontages 배열 순서대로 진행.
 * ANS_ComboWindow 구간에서 스킬 입력 시 다음 단계 몽타주를 즉시 재생.
 * 타겟 방향 회전은 ANS_RotateToTarget이 담당.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Skill : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_Skill();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 콤보 몽타주 배열. 배열 순서대로 콤보가 진행된다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

private:
	/** 현재 콤보 몽타주를 재생한다. 기존 몽타주 태스크가 있으면 정리 후 교체 */
	void PlayComboMontage();

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

	int32 CurrentComboIndex = 0;
};
