// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxGameplayAbility.h"
#include "WxGA_HitReact.generated.h"

class UAnimMontage;

/**
 * 피격 반응 어빌리티.
 *
 * 사용 흐름:
 *  1. 데미지 수신 → AttributeSet이 Event.HitReact 이벤트 발송
 *  2. GameplayEvent 트리거 → ActivateAbility → HitReactMontage 재생
 *  3. 몽타주 완료/중단 → EndAbility
 *
 * State.Dead 시 활성화 차단.
 */
UCLASS()
class WXCOMBAT_API UWxGA_HitReact : public UWxGameplayAbility
{
	GENERATED_BODY()

public:
	UWxGA_HitReact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> HitReactMontage;

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
