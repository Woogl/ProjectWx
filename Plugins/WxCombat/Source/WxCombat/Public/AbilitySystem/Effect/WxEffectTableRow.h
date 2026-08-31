// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxEffectTableRow.generated.h"

/** RowName 예시: GE_Exceed */
USTRUCT(BlueprintType)
struct WXCOMBAT_API FWxEffectTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 배율인지 가산치인지는 이 값을 쓰는 모디파이어의 ModifierOp이 정한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	float Magnitude = 0.f;

	/** 지속시간(초). HasDuration GE만 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "0.0"))
	float Duration = 0.f;
};
