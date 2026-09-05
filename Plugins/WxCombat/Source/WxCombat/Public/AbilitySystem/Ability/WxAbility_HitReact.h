// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_HitReact.generated.h"

class UAnimMontage;

/**
 * 데미지 파이프라인이 보내는 Event.Hit으로 트리거되어 TargetTags의 HitReact.*에 매칭되는 몽타주를 재생한다.
 *
 * 평타로는 경직이 나지 않는다 — 반응 없는 히트는 이 어빌리티가 즉시 종료한다.
 * 가드 중 피격 반응은 WxAbility_GuardReact가 맡는다 — 그쪽이 Ability.Guard를 요구하고 이쪽이 같은 태그에 막히므로 한 히트에 둘 중 하나만 뜬다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_HitReact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_HitReact();

	/** 피격 몽타주는 길이가 곧 경직 시간이므로 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 종류별 몽타주를 지정하지 않았을 때의 폴백이기도 하다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> NormalHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockbackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockdownMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> KnockupMontage;

	/** 이동 튜닝(JumpZVelocity)과 분리해 전투 쪽에서 정한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability", meta = (ClampMin = "0.0"))
	float KnockupZVelocity = 640.f;

	/** 공격이 퍼펙트 가드로 막혀 공격자가 경직될 때 재생. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> ParryReactMontage;

private:
	UAnimMontage* SelectMontage(FGameplayTag ReactionTag) const;

	void FaceInstigator(AActor* AvatarActor, const AActor* Instigator);
};
