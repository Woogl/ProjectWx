// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Skill.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * 스킬 어빌리티.
 *
 * 입력 시 단일 몽타주를 재생하고, 완료 또는 중단 시 EndAbility.
 * 타겟 방향 회전은 ANS_TurnAround이 담당.
 *
 * 쿨다운/충전 시스템은 어빌리티 BP의 CooldownGameplayEffectClass 슬롯에
 * UWxEffect_CooldownBase 기반 BP 에셋을 지정해 설정한다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Skill : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Skill();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> SkillMontage;

	/** MP 소모량. 0 이하이면 코스트 미적용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Cost")
	float MPCost = 0.f;

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

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
