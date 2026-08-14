// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxMMC_CooldownDuration.generated.h"

/**
 * 충전이 직렬로 회복되도록 이 어빌리티의 활성 쿨다운 GE 중 최장 잔여시간을 더한다.
 * 신규 GE는 이 시점에 아직 미적용이라 기존 것만 집계된다.
 */
UCLASS()
class WXCOMBAT_API UWxMMC_CooldownDuration : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
