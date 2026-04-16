// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_DrainDP.generated.h"

/**
 * 지속시간 동안 DP를 서서히 소진하는 GameplayEffect.
 *
 * HasDuration 정책으로 SetByCaller.Duration 동안 DrainPeriod 간격으로 DP를 차감한다.
 * 틱당 차감량은 SetByCaller.DrainDP로 전달하며, 호출 어빌리티가 MaxDP * DrainPeriod / Duration 음수로 계산한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_DrainDP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_DrainDP();

	/** 드레인 틱 간격(초) : 1/30초 */
	static constexpr float DrainPeriod = 1.0f / 30.0f;
};
