// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Guard.generated.h"

class UAnimMontage;

/**
 * 가드 홀드 상태를 소유한다 — 경감 효과 부여와 루핑 자세 몽타주가 전부다. 효과는 GA_Guard의 ActivationOwnedEffects가 지목한다.
 *
 * 피격·퍼펙트 가드 연출은 UWxAbility_GuardReact가 맡는다. 그쪽이 몽타주 슬롯을 가져가면 자세를 양보했다가, 끝나면 다시 세우거나 그 사이 키를 뗐으면 종료한다.
 * 가드가 SP 고갈로 깨질 때도 그 어빌리티가 이 어빌리티를 끊고 브레이크 연출을 완주시킨다.
 *
 * Damage.CanGuard가 없는 피격은 퍼펙트 가드 윈도우 중이라도 가드로 막히지 않는다.
 * UWxCombatAttributeSet::ProcessDamageTaken이 이 어빌리티를 Cancel한 뒤 Event.Hit을 보낸다.
 *
 * 가드 반격은 아직 성립하지 않는다. 이 어빌리티가 배타 점유를 놓지 않고(가드 몽타주에 StartRecovery가 없다) 공격 쪽도 Ability.Guard를 지목하지 않아 가드 중에는 공격이 발동하지 못하며, Effect.GuardReduction으로 반격 세트를 고르는 공격 어빌리티도 아직 없다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Guard : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Guard();

	/** 홀드 입력이라 키가 눌려 있을 때만 시작한다. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	/** 자세 루프가 공격 속도에 흔들리면 안 되므로 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 루핑하는 자세 몽타주라 완주로 끝나지 않는다. */
	virtual void HandleMontageCompleted() override;

	/** 리액션이 슬롯을 가져간 것뿐이면 가드를 끊지 않는다. */
	virtual void HandleMontageInterrupted() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> GuardMontage;

private:
	/** 리액션이 붙어 있는 지금 걸어야 한다 — 태그가 없는 상태로 걸면 태스크가 즉시 해제로 발화한다. */
	void ListenForGuardReactEnded();

	UFUNCTION()
	void HandleGuardReactEnded();

	/**
	 * 원격 서버 인스턴스는 판단하지 않는다 — 스펙의 키 상태를 발동 RPC가 true로 세운 뒤 릴리즈로 내려주는 경로가 없어 늘 눌린 것으로 보인다.
	 * 그쪽은 소유 클라가 복제하는 종료로 정리된다.
	 */
	bool IsInputHeld() const;
};
