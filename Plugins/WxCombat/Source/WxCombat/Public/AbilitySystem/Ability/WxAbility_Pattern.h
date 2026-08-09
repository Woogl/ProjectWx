// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Pattern.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * BT/AI가 TryActivateAbility로 직접 발동해 단일 몽타주를 재생한다.
 * 입력·UI 아이콘은 쓰지 않으며, 쿨다운·충전은 WxAbilityBase의 AbilityDataRow로 설정한다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Pattern : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Pattern();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> Montage;

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
