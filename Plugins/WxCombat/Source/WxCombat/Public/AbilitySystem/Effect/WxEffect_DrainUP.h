// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_DrainUP.generated.h"

/**
 * 지속시간 동안 UP를 서서히 소진하는 GameplayEffect.
 *
 * HasDuration 정책으로 SetByCaller.Duration 동안 1/30초 간격으로 UP를 차감한다.
 * 틱당 차감량은 MaxUP * (DrainPeriod / Duration) 으로,
 * Duration이 지나면 UP가 정확히 0에 도달한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_DrainUP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_DrainUP();

	/** 드레인 틱 간격(초) : 1/30초 */
	static constexpr float DrainPeriod = 1.0f / 30.0f;
};
