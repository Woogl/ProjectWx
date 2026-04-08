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
 *  1. 데미지 수신 → Event.HitReact[.Knockback|.Knockdown|.Knockup] 이벤트 발송
 *  2. GameplayEvent 트리거 → ActivateAbility
 *  3. 트리거 태그에 매칭되는 몽타주 재생 → 완료 시 EndAbility
 *
 * 가드 중 피격 반응은 WxAbility_Guard가 직접 처리하므로,
 * State.Guard 활성 시 이 어빌리티는 활성화되지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_HitReact : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_HitReact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 기본 피격 반응 몽타주. Event.HitReact 트리거 시 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> HitReactMontage;

	/** 넉백 몽타주. Event.HitReact.Knockback 트리거 시 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockbackMontage;

	/** 넉다운 몽타주. Event.HitReact.Knockdown 트리거 시 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockdownMontage;

	/** 넉업 몽타주. Event.HitReact.Knockup 트리거 시 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockupMontage;

private:
	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
};
