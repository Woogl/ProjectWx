// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility.h"
#include "WxAbility_Guard.generated.h"

class UAnimMontage;

/**
 * 가드 어빌리티.
 *
 * 사용 흐름:
 *  1. 입력 홀드 → ActivateAbility → GuardMontage 재생, ANS.Guard 태그 부여
 *  2. 몽타주의 ANS_Guard 구간 동안 가드 판정 활성
 *  3. 가드 중 피격 → Event.HitReact 수신 → GuardHitReactMontage 재생 → GuardMontage로 복귀
 *  4. 입력 릴리즈 → InputReleased → EndAbility → ANS.Guard 태그 제거
 *
 * ANS.Guard 태그는 어빌리티 활성 동안 유지되므로
 * 몽타주 전환(가드→가드피격→가드) 중에도 가드 판정이 끊기지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Guard : public UWxAbility
{
	GENERATED_BODY()

public:
	UWxAbility_Guard();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardBreakMontage;

private:
	bool IsPlayingGuardBreakMontage() const;
	void PlayGuardMontage();
	void PlayGuardBreakMontage();
	void ListenForHitReact();

	UFUNCTION()
	void HandleGuardHitReact(FGameplayEventData Payload);

	void HandlePPChanged(const FOnAttributeChangeData& ChangeData);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageBlendOut();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	/** GuardHitReactMontage 재생 중인지 여부 */
	bool bPlayingHitReact = false;

	/** 몽타주 전환(가드→가드피격)에 의한 인터럽트를 무시하기 위한 플래그 */
	bool bMontageSwapInProgress = false;
};
