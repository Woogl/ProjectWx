// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxAbilityTableRow.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EWxAbilityCostResource : uint8
{
	None,
	SP,
	MP,
	UP,
};

/** RowName 예시: GA_Skill_E, GA_Ultimate */
USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxAbilityTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 쿨다운 시간(초). 0 이하이면 쿨다운 미적용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
	float CooldownTime = 0.f;

	/** 1이면 단일 쿨다운 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
	int32 MaxRecharges = 1;

	/** None이면 코스트 미적용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	EWxAbilityCostResource CostResource = EWxAbilityCostResource::None;

	/** CostResource 자원의 소모량. 질주처럼 지속 소모하는 어빌리티에서는 진입 비용이다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0.0"))
	float CostAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Icon;
};
