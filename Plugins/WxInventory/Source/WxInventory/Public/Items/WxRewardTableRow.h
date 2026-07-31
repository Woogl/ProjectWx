// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "WxRewardTableRow.generated.h"

class UWxItemDefinition;

/**
 * 단일 보상 항목. 지급할 아이템 정의와 수량의 Pair.
 *
 * Item 은 TSoftObjectPtr 로 지연 로드된다.
 * 보상 테이블을 로드해도 참조 아이템 자산까지 강참조로 끌어오지 않으며, 실제 지급 시점에 로드한다.
 */
USTRUCT(BlueprintType)
struct WXINVENTORY_API FWxItemRewardEntry
{
	GENERATED_BODY()

	/** 지급할 아이템 정의. 비어 있으면 이 항목은 무시된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TSoftObjectPtr<UWxItemDefinition> Item;

	/** 지급 수량. 최소 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;

	/** 아이템이 지정되어 있고 수량이 1 이상이면 유효하다. */
	bool IsValid() const;
};

/**
 * 아이템 보상 데이터테이블 Row 구조체.
 * 지급할 (아이템, 수량) Pair 를 최대 5개까지 정의한다.
 * 빈(아이템 미지정) 슬롯은 지급 시 무시되므로 1~5개를 자유롭게 채우면 된다.
 * RowName 예시: Reward_Quest_01, Reward_Chest_Rare
 */
USTRUCT(BlueprintType)
struct WXINVENTORY_API FWxRewardTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 보상 항목 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FWxItemRewardEntry Reward1;

	/** 보상 항목 2. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FWxItemRewardEntry Reward2;

	/** 보상 항목 3. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FWxItemRewardEntry Reward3;

	/** 보상 항목 4. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FWxItemRewardEntry Reward4;

	/** 보상 항목 5. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	FWxItemRewardEntry Reward5;

	/**
	 * 유효한(아이템이 지정된) 보상 항목만 순서대로 수집한다.
	 * 빈 슬롯은 건너뛴다.
	 */
	void GetValidRewards(TArray<FWxItemRewardEntry>& OutRewards) const;
};
