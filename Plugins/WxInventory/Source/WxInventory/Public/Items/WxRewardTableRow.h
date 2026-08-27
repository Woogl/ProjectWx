// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "WxRewardTableRow.generated.h"

class UWxItemDefinition;

/**
 * Item 이 TSoftObjectPtr 라 보상 테이블을 로드해도 아이템 자산까지 끌어오지 않고, 실제 지급 시점에 로드한다.
 */
USTRUCT(BlueprintType)
struct WXINVENTORY_API FWxItemRewardEntry
{
	GENERATED_BODY()

	/** 비어 있으면 이 항목은 무시된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TSoftObjectPtr<UWxItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;

	bool IsValid() const;
};

USTRUCT(BlueprintType)
struct WXINVENTORY_API FWxRewardTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FWxItemRewardEntry Reward1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FWxItemRewardEntry Reward2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FWxItemRewardEntry Reward3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FWxItemRewardEntry Reward4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FWxItemRewardEntry Reward5;

	void GetValidRewards(TArray<FWxItemRewardEntry>& OutRewards) const;
};
