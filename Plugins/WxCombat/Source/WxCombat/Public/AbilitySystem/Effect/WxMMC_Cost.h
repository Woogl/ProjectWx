// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxMMC_Cost.generated.h"

struct FWxAbilityTableRow;

/**
 * 어빌리티 코스트 MMC 공용 베이스.
 *
 * 소스 어빌리티(UWxAbilityBase)의 AbilityDataRow를 계산 시점에 조회해 코스트 값을 만든다.
 * 어트리뷰트는 캡처하지 않는다(RelevantAttributesToCapture 비움).
 *
 * MMC API가 평가 중인 Modifier 인덱스를 주지 않아, MP/UP는 파생 클래스로 분리한다(UWxMMC_MPCost/UWxMMC_UPCost).
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxMMC_Cost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

protected:
	/** Spec 컨텍스트의 소스 어빌리티에서 코스트 Row를 해석한다. 어빌리티/Row 미가용 시 nullptr. */
	const FWxAbilityTableRow* GetCostRow(const FGameplayEffectSpec& Spec) const;
};

/** MP 코스트. AbilityDataRow의 MPCost를 음수로 반환(자원 감산). */
UCLASS()
class WXCOMBAT_API UWxMMC_MPCost : public UWxMMC_Cost
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};

/** UP 코스트. AbilityDataRow의 UPCost를 음수로 반환(자원 감산). */
UCLASS()
class WXCOMBAT_API UWxMMC_UPCost : public UWxMMC_Cost
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
