// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility_Combo.h"
#include "WxAbility_Pattern.generated.h"

/**
 * BT/AI가 TryActivateAbility로 직접 발동해 ComboMontages를 배열 순서대로 재생한다.
 * 앞 단의 블렌드아웃에서 다음 단을 걸어, 한 번의 발동이 모든 단계를 재생한다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Pattern : public UWxAbility_Combo
{
	GENERATED_BODY()

public:
	UWxAbility_Pattern();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	virtual void HandleMontageBlendOut() override;
};
