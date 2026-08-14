// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityTableRow.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxMMC_Cost.generated.h"

/**
 * 소스 어빌리티(UWxAbilityBase)의 AbilityDataRow를 계산 시점에 조회해 코스트 값을 만든다.
 * 어트리뷰트는 캡처하지 않는다(RelevantAttributesToCapture 비움).
 *
 * MMC API가 평가 중인 Modifier 인덱스를 주지 않아, 자원별로 파생 클래스를 분리한다(UWxMMC_MPCost/UWxMMC_UPCost/UWxMMC_SPCost).
 * Row가 고른 자원과 일치하는 파생 클래스만 값을 내고 나머지는 0이다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxMMC_Cost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

protected:
	/** 자원 감산이라 CostAmount를 음수로 반환한다 */
	float GetCostMagnitude(const FGameplayEffectSpec& Spec, EWxAbilityCostResource Resource) const;
};

UCLASS()
class WXCOMBAT_API UWxMMC_MPCost : public UWxMMC_Cost
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};

UCLASS()
class WXCOMBAT_API UWxMMC_UPCost : public UWxMMC_Cost
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};

UCLASS()
class WXCOMBAT_API UWxMMC_SPCost : public UWxMMC_Cost
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
