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
 * 입력 시 단일 몽타주를 재생하고, 완료 또는 중단 시 EndAbility.
 * 타겟 방향 회전은 ANS_TurnAround이 담당.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Skill : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_Skill();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> SkillMontage;

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
