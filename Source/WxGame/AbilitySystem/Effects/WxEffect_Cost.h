// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxEffect_Cost.generated.h"

enum class EWxAbilityCostResource : uint8;

/**
 * 값은 각 MMC가 계산 시점에 소스 어빌리티(UWxAbilityBase) CDO의 CostResource·CostAmount를 읽어 만든다.
 * 그래서 UWxAbilityBase는 Effect.IgnoreCosts 우회만 얹고 순정 CheckCost(CanApplyAttributeModifiers)/ApplyCost를 그대로 쓴다.
 * UWxAbilityBase의 CostGameplayEffectClass 기본값이다.
 */
UCLASS()
class WXGAME_API UWxEffect_Cost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cost();
};

/**
 * MMC API가 평가 중인 Modifier 인덱스를 주지 않아, 자원별로 파생 클래스를 분리한다.
 * 어빌리티가 고른 자원과 일치하는 파생 클래스만 값을 내고 나머지는 0이다.
 */
UCLASS(Abstract)
class WXGAME_API UWxMMC_Cost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

protected:
	/** 자원 감산이라 CostAmount를 음수로 반환한다 */
	float GetCostMagnitude(const FGameplayEffectSpec& Spec, EWxAbilityCostResource Resource) const;
};

UCLASS()
class WXGAME_API UWxMMC_MPCost : public UWxMMC_Cost
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};

UCLASS()
class WXGAME_API UWxMMC_UPCost : public UWxMMC_Cost
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};

UCLASS()
class WXGAME_API UWxMMC_SPCost : public UWxMMC_Cost
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
