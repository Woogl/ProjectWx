// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Sprint.generated.h"

struct FOnAttributeChangeData;

/**
 * 입력을 누르는 동안 SPD 배율 GameplayEffect를 걸어 두고, 떼면 걷어낸다.
 * 실제로 이동하는 동안에는 State.Sprint를 부여해 SP를 소모시키고 자연 회복을 멈추며, SP가 바닥나면 스스로 종료한다.
 *
 * 진입 비용은 AbilityDataRow의 코스트(SP)다 — 순정 CheckCost가 발동을 막으므로, 값을 올리면 고갈 직후 재발동에 필요한 회복량도 그만큼 늘어난다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Sprint : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Sprint();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleMovingChanged(bool bIsMoving);

	void HandleSPChanged(const FOnAttributeChangeData& ChangeData);

	/** SPD에 곱해지는 이동 속도 배율(1.0 = 평상시 속도) */
	UPROPERTY(EditDefaultsOnly)
	float SprintSpeedScale = 1.5f;

	FActiveGameplayEffectHandle SpeedEffectHandle;

	FActiveGameplayEffectHandle DrainEffectHandle;

	FDelegateHandle SPChangedHandle;
};
