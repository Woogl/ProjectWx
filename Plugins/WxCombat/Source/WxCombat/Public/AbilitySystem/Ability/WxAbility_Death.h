// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Death.generated.h"

class UAnimMontage;

/**
 * HP가 0이 되어 붙는 State.Dead 태그로 발동한다.
 * DeathMontage가 있으면 그 포즈로 끝내고, 없거나 외부에 끊기면 래그돌로 폴백한다.
 *
 * 래그돌은 어빌리티 인스턴스가 없는 시뮬 프록시·late joiner도 커버해야 한다.
 * 그래서 서버가 State.Ragdoll 루스 태그만 발행(TagOnly 복제)하고, 전 머신의 캐릭터가 그 태그를 보고 스스로 전환한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Death : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Death();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DeathMontage;

private:
	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
	
	void PlayDeathMontageOrRagdoll();

	void RagdollAndEnd(bool bWasCancelled);

	void EnableRagdoll();
};
