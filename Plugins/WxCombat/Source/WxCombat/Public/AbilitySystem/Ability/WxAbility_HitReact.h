// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_HitReact.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

/**
 * 피격 반응 어빌리티.
 *
 * 사용 흐름:
 *  1. 데미지 수신 → Event.HitReact[.Knockback|.Knockdown|.Knockup] 이벤트 발송
 *  2. GameplayEvent 트리거 → ActivateAbility
 *  3. 트리거 태그에 매칭되는 몽타주 재생 → 완료 시 EndAbility
 *
 * 재생 중 다른 종류의 HitReact 이벤트가 도착하면(예: Normal 재생 중 Knockback),
 * bRetriggerInstancedAbility로 EndAbility 후 ActivateAbility가 재진입하며,
 * CurrentMontageTask를 명시적으로 정리해 이전 태스크의 잔여 콜백이 새 재생을
 * 즉시 종료시키는 레이스를 방지한다.
 *
 * 가드 중 피격 반응은 WxAbility_Guard가 직접 처리하므로,
 * State.Guard 활성 시 이 어빌리티는 활성화되지 않는다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_HitReact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_HitReact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

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
	bool PlayHitReactMontage(UAnimMontage* Montage);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	UFUNCTION()
	void HandleMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;
};
