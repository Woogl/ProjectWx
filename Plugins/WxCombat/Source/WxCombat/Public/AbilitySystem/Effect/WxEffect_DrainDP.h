// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxEffect_DrainDP.generated.h"

/**
 * HasDuration 정책으로 SetByCaller.Duration 동안 DrainPeriod 간격으로 DP를 차감한다.
 * 틱당 차감량은 UWxMMC_DrainDP가 계산하며, Duration이 지나면 DP가 정확히 0에 도달한다.
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

/**
 * MaxDP가 GE Duration 동안 선형으로 0까지 빠지도록 틱당 차감량을 낸다.
 *
 * Duration은 GE Spec에서 자동으로 읽으므로, 호출부는 SetByCaller.Duration만 세팅하면 된다.
 * 캡처는 non-snapshot이라 매 틱 현재 MaxDP를 재반영한다.
 */
UCLASS()
class WXCOMBAT_API UWxMMC_DrainDP : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UWxMMC_DrainDP();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition MaxDPCaptureDef;
};
