// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityTableRow.h"
#include "GameplayEffect.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxEffect_Cost.generated.h"

/**
 * 값은 각 MMC가 계산 시점에 소스 어빌리티(UWxAbilityBase)의 AbilityDataRow를 조회해 만든다.
 * CDO가 완전 자기완결이라 엔진 순정 CheckCost(CanApplyAttributeModifiers)/ApplyCost/GetCostGameplayEffect를 그대로 사용한다.
 *
 * 아울러 어빌리티 CostGameplayEffectClass의 "프로젝트 방식(테이블 기반)" 기본 클래스를 겸한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cost();
};

/**
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
