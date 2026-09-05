// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxEffect_DrainGP.generated.h"

/** 지속시간은 스펙의 SetDuration으로 직접 받는다 — 잠가서 넣어야 적용 시점의 Def 기반 재계산이 덮어쓰지 않는다. */
UCLASS()
class WXCOMBAT_API UWxEffect_DrainGP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_DrainGP();

	/** 드레인 틱 간격(초) */
	static constexpr float DrainPeriod = 1.0f / 30.0f;
};

/**
 * MaxGP가 GE Duration 동안 선형으로 0까지 빠지도록 틱당 차감량을 낸다.
 *
 * 틱이 주기 배수에만 놓이는 탓에 시간이 아니라 실제 실행 횟수로 나누며, Duration이 주기의 배수가 아니면 마지막 틱은 최대 한 주기 이르다.
 * 캡처는 non-snapshot이라 매 틱 현재 MaxGP를 재반영한다.
 */
UCLASS()
class WXCOMBAT_API UWxMMC_DrainGP : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UWxMMC_DrainGP();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition MaxGPCaptureDef;
};
