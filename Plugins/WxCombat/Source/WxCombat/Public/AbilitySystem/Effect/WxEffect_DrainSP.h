// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_DrainSP.generated.h"

/**
 * Movement.Sprint를 보유한 동안에만 틱한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_DrainSP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_DrainSP();

	/** MaxSP에서 0까지 소진되는 데 걸리는 시간(초) */
	static constexpr float FullDrainDuration = 8.0f;

	/** 소모 틱 간격(초) */
	static constexpr float DrainPeriod = 1.0f / 30.0f;
};
