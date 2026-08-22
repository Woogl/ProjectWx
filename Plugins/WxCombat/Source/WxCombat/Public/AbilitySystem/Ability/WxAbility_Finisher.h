// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Finisher.generated.h"

class UAnimMontage;
struct FGameplayEventData;

/**
 * 피니셔 어빌리티 — 공격자(플레이어) 측.
 * 한 클래스가 두 변형을 트리거 EventTag로 분기한다.
 *  - 앞잡(피니셔): Event.Finisher 트리거, 짝 피격 Event.HitReact.Finisher
 *  - 뒤잡(백스탭): Event.Backstab 트리거, 짝 피격 Event.HitReact.Backstab
 *
 * 상호작용(서버 권위)이 보내는 GameplayEvent로 트리거되며, 피해자 위치를 공유 앵커로 모션워핑 정렬한 뒤 양쪽 몽타주를 동시에 재생한다.
 *
 * 변형별로 다른 것은 공격 몽타주와 짝 피격 태그 둘뿐이다.
 * 대미지는 두 변형 모두 노티파이 DamageDataRow의 계수를 쓰므로 수치도 타이밍도 노티파이가 소유한다.
 * 그로기 해제(DP 0)는 종료 시 대상에 UWxEffect_ResetDP를 적용해 처리한다 — 몽타주가 어떻게 끝나든 한 번 적용된다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Finisher : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Finisher();

	/** 공격 몽타주의 WxAnimNotify_FinisherDamage가 대미지 프레임에 호출한다. */
	void ApplyFinisherDamage(const FDataTableRowHandle& DamageInfo) const;

	/** 피해자 짝 피격이 고정 1.0으로 재생되므로 공격자도 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 앞잡(Event.Finisher) 공격 몽타주. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> FinisherMontage;

	/** 뒤잡(Event.Backstab) 공격 몽타주. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> BackstabMontage;

private:
	void RegisterWarpTarget(AActor* AvatarActor, const AActor* Target) const;

	TWeakObjectPtr<const AActor> TargetActor;
};
