// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_DrainDP.generated.h"

/**
 * 지속시간 동안 DP를 서서히 소진하는 GameplayEffect.
 *
 * HasDuration 정책으로 SetByCaller.Duration 동안 DrainPeriod 간격으로 DP를 차감한다.
 * 틱당 차감량은 UWxMMC_LinearDrain이 계산하며, Duration이 지나면 DP가 정확히 0에 도달한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_DrainDP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_DrainDP();

	/** 드레인 틱 간격(초) */
	static constexpr float DrainPeriod = 1.0f / 30.0f;
};
