// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxMMC_LinearDrain.generated.h"

/**
 * 대상 Max 속성을 GE Duration 동안 선형적으로 0까지 소진시키는 MMC.
 *
 * Duration은 GE Spec에서 자동으로 읽으므로, 호출부는 SetByCaller.Duration만 세팅하면 된다.
 * GE Modifier.Attribute가 DP면 MaxDP, UP면 MaxUP를 캡처한다.
 * 캡처는 non-snapshot이라 매 틱 현재 Max 값을 재반영한다.
 *
 * 제약: Modifier가 정확히 1개인 GE에서만 유효하다.
 * MMC API가 평가 중인 Modifier 인덱스를 알려주지 않아 복수 Modifier는 구분 불가.
 */
UCLASS()
class WXCOMBAT_API UWxMMC_LinearDrain : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UWxMMC_LinearDrain();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition MaxDPCaptureDef;

	FGameplayEffectAttributeCaptureDefinition MaxUPCaptureDef;
};
