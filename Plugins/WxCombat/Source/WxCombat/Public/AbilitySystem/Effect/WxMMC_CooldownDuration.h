// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxMMC_CooldownDuration.generated.h"

/**
 * 쿨다운 GE의 Duration을 계산하는 MMC.
 *
 * 소스 어빌리티(UWxAbilityBase)의 AbilityDataRow.CooldownTime을 계산 시점에 조회하고,
 * 직렬 충전 회복을 위해 시전 ASC의 활성 UWxEffect_Cooldown GE 중 이 어빌리티 것의 최장 잔여시간을 더한다.
 * 반환값 = LongestRemaining + CooldownTime. (신규 GE는 duration 계산 시점에 아직 미적용이라 기존 것만 집계된다.)
 *
 * 어트리뷰트는 캡처하지 않는다(RelevantAttributesToCapture 비움).
 */
UCLASS()
class WXCOMBAT_API UWxMMC_CooldownDuration : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
