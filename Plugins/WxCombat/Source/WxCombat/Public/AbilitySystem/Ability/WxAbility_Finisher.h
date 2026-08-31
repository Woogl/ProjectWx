// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Finisher.generated.h"

class UAnimMontage;
struct FGameplayEventData;

USTRUCT()
struct FWxFinisherVariant
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> AttackerMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Ability")
	TObjectPtr<UAnimMontage> VictimMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Ability", meta = (RowType = "/Script/WxCombat.WxDamageTableRow"))
	FDataTableRowHandle DamageDataRow;
};

/**
 * 피니셔 어빌리티 — 공격자(플레이어) 측. 그로기 대상의 앞잡과 비전투 후방 대상의 뒤잡을 모두 처리한다.
 *
 * 상호작용(서버 권위)이 보내는 GameplayEvent로 트리거되며, 피해자 위치를 공유 앵커로 모션워핑 정렬한 뒤 양쪽 몽타주를 동시에 시작한다.
 * 피해자 쪽은 UWxAbility_PlayMontageOnce를 일회성으로 부여해 재생시키므로 피해자에게 상시 부여된 수신 어빌리티가 필요 없다.
 *
 * 대미지는 변형의 DamageDataRow 계수를 쓰며, 몽타주 노티파이가 실행 타이밍을 정한다.
 * GP 초기화는 종료 시 대상에 UWxEffect_ResetGP를 적용해 처리한다 — 앞잡·뒤잡 모두 몽타주가 어떻게 끝나든 한 번 적용된다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Finisher : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Finisher();

	bool IsBackstab() const;

	/** 피해자 짝 피격이 고정 1.0으로 재생되므로 공격자도 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 그로기 대상의 앞잡 연출과 피해 설정. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Ability")
	FWxFinisherVariant FinisherVariant;

	/** 비그로기 후방 대상의 뒤잡 연출과 피해 설정. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Ability")
	FWxFinisherVariant BackstabVariant;

private:
	const FWxFinisherVariant& GetCurrentVariant() const;
	void RegisterWarpTarget(AActor* AvatarActor, const AActor* Target) const;

	TWeakObjectPtr<const AActor> TargetActor;

	bool bBackstab = false;
};
