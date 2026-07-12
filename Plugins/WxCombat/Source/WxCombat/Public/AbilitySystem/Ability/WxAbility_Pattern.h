// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Pattern.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * 적 공격 패턴 어빌리티.
 *
 * BT/AI에서 TryActivateAbility로 직접 발동한다.
 * 입력/UI 아이콘는 사용하지 않으며, 디테일 패널에서 해당 프로퍼티는 비활성화된다.
 *
 * 단일 몽타주를 재생하고 완료 또는 중단 시 EndAbility.
 * 쿨다운/충전은 WxAbilityBase의 CooldownTime, MaxRecharges로 설정한다.
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
