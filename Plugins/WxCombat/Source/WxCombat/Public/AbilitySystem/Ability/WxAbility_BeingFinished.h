// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_BeingFinished.generated.h"

/**
 * 처형 당하기 — 피해자 측.
 * 상시 부여되지 않는다. 공격자의 처형 어빌리티(UWxAbility_Finisher·UWxAbility_Backstab)가 GiveAbilityAndActivateOnce로 부여하며 곧바로 발동시키고, 몽타주가 끝나 종료되면 스펙도 걷힌다.
 * 재생할 몽타주는 트리거 이벤트의 OptionalObject로 받는다 — 짝 몽타주는 공격자 에셋이 쌍으로 소유한다.
 * 활성 구간 동안 Ability.BeingFinished를 발행해 재처형 프롬프트를 닫는다 — 기상까지 포함한 피해자 몽타주 길이가 곧 그 수명이다.
 */
UCLASS()
class WXCOMBAT_API UWxAbility_BeingFinished : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_BeingFinished();

	/** 공격자 몽타주와 프레임 싱크를 맞춰야 하므로 ASPD를 반영하지 않는다. */
	virtual float GetMontagePlayRate() const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
