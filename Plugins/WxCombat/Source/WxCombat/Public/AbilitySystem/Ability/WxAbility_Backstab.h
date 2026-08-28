// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Backstab.generated.h"

class UAnimMontage;
struct FGameplayEventData;

/**
 * 뒤잡 어빌리티 — 공격자(플레이어) 측. 앞잡(UWxAbility_Finisher)과 같은 상호작용 이벤트를 받되 그로기가 아닌 대상에게만 성립한다.
 *
 * 피해자 위치를 공유 앵커로 모션워핑 정렬한 뒤, 피해자에게 UWxAbility_BeingFinished를 일회성으로 부여해 양쪽 몽타주를 동시에 시작한다.
 * 대미지는 노티파이 DamageDataRow의 계수를 쓰므로 수치도 타이밍도 노티파이가 소유한다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Backstab : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Backstab();

	/** 공격 몽타주의 WxAnimNotify_FinisherDamage가 대미지 프레임에 호출한다. */
	void ApplyBackstabDamage(const FDataTableRowHandle& DamageInfo) const;

	/** 피해자 짝 피격이 고정 1.0으로 재생되므로 공격자도 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> AttackerMontage;

	/** 공격자 몽타주와 프레임 싱크되도록 같은 길이로 제작한다. 피해자가 공격자를 향해 회전한 뒤 재생한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> VictimMontage;

private:
	void RegisterWarpTarget(AActor* AvatarActor, const AActor* Target) const;

	TWeakObjectPtr<const AActor> TargetActor;
};
