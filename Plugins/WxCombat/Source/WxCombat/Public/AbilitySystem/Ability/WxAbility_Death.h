// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Death.generated.h"

class UAnimMontage;

/**
 * 사망 어빌리티.
 *
 * 사용 흐름:
 *  1. HP == 0 → PostGameplayEffectExecute에서 State.Dead 태그 부여
 *  2. OwnedTagPresent 트리거 → ActivateAbility
 *  3. DeathMontage 유효 → DeathMontage 재생 → EndAbility (의도한 사망 포즈 유지, 래그돌 X)
 *     DeathMontage 무효 → 짧은 지연 후 래그돌 활성화 → EndAbility (활성 HitReact 몽타주가 잘리지 않게 인계)
 *
 * DeathMontage가 외부 시스템에 의해 중단되면 안전 폴백으로 래그돌 활성화.
 * 래그돌은 어빌리티 인스턴스가 없는 시뮬 프록시·late joiner도 커버해야 하므로, 서버가 State.Ragdoll 루스 태그를 발행(TagOnly 복제)하고 전 머신의 캐릭터가 감지해 자체 래그돌 전환을 수행한다.
 * 사망 발동 시 신규 어빌리티 차단.
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

	void HandleRagdollDelayElapsed();

	void RagdollAndEnd(bool bWasCancelled);

	void EnableRagdoll();

	FTimerHandle RagdollDelayTimerHandle;
};
