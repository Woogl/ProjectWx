// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxCombatAttributeInitTableRow.generated.h"

USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxCombatAttributeInitTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = "0.0"))
	float HP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = "0.0"))
	float MaxHP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = "0.0"))
	float SP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = "0.0"))
	float MaxSP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = "0.0"))
	float DP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = "0.0"))
	float MaxDP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource", meta = (ClampMin = "0.0"))
	float MP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource", meta = (ClampMin = "0.0"))
	float MaxMP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource", meta = (ClampMin = "0.0"))
	float UP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource", meta = (ClampMin = "0.0"))
	float MaxUP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float ATK = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float DEF = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float CritRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float CritDMG = 0.f;
};
