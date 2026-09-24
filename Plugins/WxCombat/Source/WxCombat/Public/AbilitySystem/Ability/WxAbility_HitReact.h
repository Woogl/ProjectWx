// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_HitReact.generated.h"

class UAnimMontage;

/**
 * 데미지 파이프라인이 보내는 Event.Hit으로 트리거되어 TargetTags의 HitReact.*와 이름이 같은 섹션을 재생한다.
 *
 * HitReact 태그 없는 일반 히트와 몽타주에 섹션이 없는 반응은 활성화 전에 거부하여 진행 중인 반응과 공격을 유지한다 — 패리는 반응 태그와 무관하게 받는다.
 * 가드 중 피격 반응은 WxAbility_GuardReact가 맡는다 — 그쪽이 Ability.Guard를 요구하고 이쪽이 같은 태그에 막히므로 한 히트에 둘 중 하나만 뜬다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_HitReact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_HitReact();

	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const override;

	/** 피격 몽타주는 길이가 곧 경직 시간이므로 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/**
	 * 섹션 이름은 반응 태그의 끝 이름(Normal, KnockBack, KnockDown, KnockUp)이다. 공격이 퍼펙트 가드로 막혀 공격자가 경직될 때는 Parry다.
	 * 슬롯 그룹이 다른 반응(가산 슬롯의 일반 피격)이나 착지 섹션을 쓰는 넉업은 한 몽타주에 합칠 수 없어 어빌리티를 따로 둔다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> HitReactMontage;

	/** 이동 튜닝(JumpZVelocity)과 분리해 전투 쪽에서 정한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability", meta = (ClampMin = "0.0"))
	float KnockupZVelocity = 640.f;

private:
	/** 섹션을 고를 반응. 패리는 Event.Hit.Parry로 돌려준다. */
	static FGameplayTag GetReactionTag(const FGameplayEventData& Payload);

	void FaceInstigator(AActor* AvatarActor, const AActor* Instigator);
};
