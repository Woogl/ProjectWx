// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_PlayMontageOnce.generated.h"

/**
 * 밖에서 주입되는 일회성 몽타주 연출.
 * 상시 부여되지 않는다. 연출을 거는 쪽이 GiveAbilityAndActivateOnce로 부여하며 곧바로 발동시키고, 몽타주가 끝나 종료되면 스펙도 걷힌다.
 * 재생할 몽타주는 트리거 이벤트의 OptionalObject로 받는다 — 짝 연출은 거는 쪽 에셋이 쌍으로 소유한다.
 * Instigator가 실려 있으면 그쪽을 바라본다.
 * 활성 구간 동안 Ability.PlayMontageOnce를 발행하므로 연출 중인 폰을 밖에서 게이팅할 수 있다 — 처형 당하기는 기상까지가 그 구간이라 재처형 프롬프트가 닫힌다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_PlayMontageOnce : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_PlayMontageOnce();

	/** 거는 쪽 몽타주와 프레임 싱크를 맞춰야 하므로 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
