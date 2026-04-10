// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_DrainDP.generated.h"

/**
 * 그로기 상태 중 DP를 서서히 소진하는 GameplayEffect.
 *
 * Duration 정책으로 DrainDuration 동안 1/30초 간격으로 DP를 차감한다.
 * 틱당 차감량은 MaxDP * (DrainPeriod / DrainDuration) 으로,
 * DrainDuration이 지나면 DP가 정확히 0에 도달한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_DrainDP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_DrainDP();

	/** DP 드레인 총 시간(초) */
	static constexpr float DrainDuration = 5.0f;

	/** 드레인 틱 간격(초) : 1/30초 */
	static constexpr float DrainPeriod = 1.0f / 30.0f;
};
