// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_HitReact.generated.h"

class UAnimMontage;

/**
 * 데미지 파이프라인이 보내는 Event.Hit으로 트리거되어 페이로드의 반응 종류(HitReact.*)에 매칭되는 몽타주를 재생한다.
 *
 * 대미지 파이프라인은 공격이 요청한 반응 종류를 가공 없이 넘기고, 그걸로 무엇을 할지는 여기서 정한다 — 그로기 중이면 넉 계열을 Normal로 낮춘다.
 *
 * 평타로는 경직이 나지 않는다 — 대미지 행이 반응 종류를 싣지 않으면 ShouldAbilityRespondToEvent가 활성화 전에 거절한다.
 * 가드 중 피격 반응은 WxAbility_Guard가 같은 이벤트로 직접 처리하므로 Effect.Guard 중에는 이 어빌리티가 뜨지 않는다.
 * 피격 중에도 새 액션을 허용하는 캐릭터는 어빌리티 BP에서 차단·캔슬 컨테이너를 비운다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_HitReact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_HitReact();

	/** 처형 짝 피격이 공격자 몽타주와 프레임 싱크돼야 하므로 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

	/** 반응 종류가 실리지 않은 히트는 활성화 전에 거절한다 — 캔슬 태그도 클라 활성 RPC도 새지 않는다. */
	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const override;

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

	/** 공격이 퍼펙트 가드로 막혀 공격자가 경직될 때 재생. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> ParryReactMontage;

	/** 피니셔(앞잡) 짝 피격 몽타주. 공격자 몽타주와 프레임 싱크되도록 같은 길이로 제작한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> FinisherHitReactMontage;

	/** 백스탭(뒤잡) 짝 피격 몽타주. 공격자를 향해 회전한 뒤 재생한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> BackstabHitReactMontage;

private:
	UAnimMontage* SelectMontage(FGameplayTag ReactionTag) const;

	void FaceInstigator(AActor* AvatarActor, const AActor* Instigator);
};
