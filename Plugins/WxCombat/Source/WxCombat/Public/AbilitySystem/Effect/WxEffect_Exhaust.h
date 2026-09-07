// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Exhaust.generated.h"

class UAbilitySystemComponent;

/**
 * 걸린 동안 Effect.Exhausted를 부여해 SP 자연 회복을 멈춘다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Exhaust : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Exhaust();

	/** SP를 소모하면 회복이 멈추는 시간(초) */
	static constexpr float Duration = 2.0f;

	/** 이미 걸려 있으면 지속시간이 이 호출 기준으로 갱신된다 */
	static void ApplyTo(UAbilitySystemComponent* TargetASC);
};
