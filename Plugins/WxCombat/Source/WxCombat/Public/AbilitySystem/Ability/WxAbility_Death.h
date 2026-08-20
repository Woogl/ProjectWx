// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Death.generated.h"

class UAnimMontage;

/**
 * HP가 0에 닿을 때 AttributeSet이 송출하는 Event.Death로 발동한다.
 * DeathMontage가 있으면 그 포즈로 두고, 없거나 외부에 끊기면 래그돌로 폴백한다.
 *
 * 연출이 끝나도 종료하지 않는다 — 활성 동안 부여되는 Ability.Death가 곧 사망 상태이며, 어빌리티 차단·피해 차단·락온 제외·AI 추적 중단이 전부 그 태그에 걸려 있다.
 * 시체는 액터가 파괴될 때 ASC가 정리한다.
 *
 * 래그돌은 어빌리티 인스턴스가 없는 시뮬 프록시·late joiner도 커버해야 한다.
 * 그래서 서버가 Event.Ragdoll 루스 태그만 발행(TagOnly 복제)하고, 전 머신의 캐릭터가 그 태그를 보고 스스로 전환한다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_Death : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Death();

	/** 사망 연출은 공격 속도를 타지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 의도한 사망 포즈로 끝났으므로 래그돌로도, 종료로도 넘기지 않는다. */
	virtual void HandleMontageCompleted() override;

	/** 외부가 사망 몽타주를 끊은 비정상 경로 — 래그돌로 폴백한다. */
	virtual void HandleMontageInterrupted() override;
	virtual void HandleMontageCancelled() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> DeathMontage;

private:
	void PlayDeathMontageOrRagdoll();

	void EnableRagdoll();
};
