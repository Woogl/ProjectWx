// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxCombatAttributeInitTableRow.generated.h"

USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxCombatAttributeInitTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital")
	float HP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital")
	float MaxHP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital")
	float SP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital")
	float MaxSP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital")
	float DP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital")
	float MaxDP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	float MP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	float MaxMP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	float UP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	float MaxUP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float ATK = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float DEF = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float CritRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float CritDMG = 0.f;
};
