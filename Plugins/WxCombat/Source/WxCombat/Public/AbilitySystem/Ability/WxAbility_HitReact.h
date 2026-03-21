// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_HitReact.generated.h"

class UAnimMontage;

/**
 * 피격 반응 어빌리티.
 *
 * 사용 흐름:
 *  1. 데미지 수신 → Event.HitReact 이벤트 발송
 *  2. GameplayEvent 트리거 → ActivateAbility
 *  3. ANS.Guard 태그 유무에 따라 GuardHitReactMontage / HitReactMontage 분기 재생
 *  4. 몽타주 완료/중단 → EndAbility
 *
 * State.Dead, ANS.Invincible 시 활성화 차단.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_HitReact : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_HitReact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardHitReactMontage;

private:
	bool bWasGuardHitReact = false;

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
};
