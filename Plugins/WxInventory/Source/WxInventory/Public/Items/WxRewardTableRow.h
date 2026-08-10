// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "WxRewardTableRow.generated.h"

class UWxItemDefinition;

/**
 * 단일 보상 항목.
 *
 * Item 이 TSoftObjectPtr 라 보상 테이블을 로드해도 아이템 자산까지 끌어오지 않고, 실제 지급 시점에 로드한다.
 */
USTRUCT(BlueprintType)
struct WXINVENTORY_API FWxItemRewardEntry
{
	GENERATED_BODY()

	/** 지급할 아이템 정의. 비어 있으면 이 항목은 무시된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TSoftObjectPtr<UWxItemDefinition> Item;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;

	bool IsValid() const;
};

/**
 * 아이템 보상 데이터테이블 Row.
 * 빈(아이템 미지정) 슬롯은 지급 시 무시되므로 5개 중 필요한 만큼만 채우면 된다.
 * RowName 예시: Reward_Quest_01, Reward_Chest_Rare
 */
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

	/** 유효한(아이템이 지정된) 보상 항목만 순서대로 수집한다. */
	void GetValidRewards(TArray<FWxItemRewardEntry>& OutRewards) const;
};
