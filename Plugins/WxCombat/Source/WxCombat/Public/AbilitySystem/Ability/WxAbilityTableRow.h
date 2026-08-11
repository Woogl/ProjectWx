// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxAbilityTableRow.generated.h"

class UTexture2D;

/**
 * 어빌리티별 밸런스 수치(쿨다운·충전·코스트)와 UI 표시 데이터를 담는 데이터테이블 Row.
 * RowName 예시: GA_Skill_1, GA_Dodge
 */
USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxAbilityTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 쿨다운 시간(초). 0 이하이면 쿨다운 미적용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
	float CooldownTime = 0.f;

	/** 최대 충전 수. 1이면 단일 쿨다운 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
	int32 MaxRecharges = 1;

	/** MP 소모량. 0 이하이면 미적용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	float MPCost = 0.f;

	/** UP 소모량. 0 이하이면 미적용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	float UPCost = 0.f;

	/** SP 소모량. 0 이하이면 미적용. 질주처럼 지속 소모하는 어빌리티에서는 진입 비용이다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	float SPCost = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Icon;
};
