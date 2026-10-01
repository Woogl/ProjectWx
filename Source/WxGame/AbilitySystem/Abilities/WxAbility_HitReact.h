// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "WxAbility_HitReact.generated.h"

/**
 * 데미지 파이프라인이 보내는 Event.Hit으로 트리거되어 반응 태그의 끝 이름(Normal, KnockBack, KnockDown, KnockUp)을 섹션으로 쓴다.
 * 해당 섹션 없이 [반응 이름]Forward가 있으면 공용 입력 방향 선택으로 [반응 이름][방향] 섹션을 재생한다.
 * 공격이 퍼펙트 가드로 막혀 공격자가 경직될 때는 Parry를 반응 이름으로 쓴다.
 * 슬롯 그룹이 다른 반응(가산 슬롯의 일반 피격)이나 착지 섹션을 쓰는 넉업은 한 몽타주에 합칠 수 없어 어빌리티를 따로 둔다.
 *
 * HitReact 태그 없는 일반 히트와 반응 이름·[반응 이름]Forward 섹션이 모두 없는 반응은 활성화 전에 거부하여 진행 중인 반응과 공격을 유지한다 — 패리는 반응 태그와 무관하게 받는다.
 * 가드 중 피격 반응은 WxAbility_GuardReact가 맡는다 — 그쪽이 Ability.Action.Guard를 요구하고 이쪽이 같은 태그에 막히므로 한 히트에 둘 중 하나만 뜬다.
 */
UCLASS(Abstract)
class WXGAME_API UWxAbility_HitReact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_HitReact();

	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool PlayMontageInternal(UAnimMontage* Montage, FName StartSection) override;

	/** KnockBack만 사용한다. 해당 몽타주 구간에는 Disable Root Motion 노티파이가 필요하다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability|Knockback", meta = (ClampMin = "0.0", Units = "cm"))
	float KnockbackDistance = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability|Knockback", meta = (ClampMin = "0.01", Units = "s"))
	float KnockbackDuration = 0.3f;

	/** 이동 튜닝(JumpZVelocity)과 분리해 전투 쪽에서 정한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability", meta = (ClampMin = "0.0"))
	float KnockupZVelocity = 640.f;

private:
	FVector KnockbackDirection = FVector::ZeroVector;
	bool bPendingKnockback = false;

	/** 섹션을 고를 반응. 패리는 Event.Hit.Parry로 돌려준다. */
	static FGameplayTag GetReactionTag(const FGameplayEventData& Payload);

	void FaceInstigator(AActor* AvatarActor, const AActor* Instigator);
};
