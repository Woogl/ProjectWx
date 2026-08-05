// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxSubtitleTableRow.generated.h"

/**
 * 자막 한 줄 = 행 하나. 자막 1편 = 테이블 1개.
 * 대화와 달리 넘기는 사람이 없으므로 Duration 이 다 차면 스스로 NextRow 로 이어가고, 가리키는 곳이 없으면 자막이 끝난다.
 */
USTRUCT(BlueprintType)
struct FWxSubtitleTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 화자 표시명. 비우면 화자 없는 나레이션이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Subtitle")
	FText Speaker;

	/** 자막 본문. 비어 있으면 자막이 종료된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Subtitle", meta = (MultiLine = "true"))
	FText Line;

	/** 이 줄을 유지할 시간(초). 0 이하면 다음 줄로 넘어가지 않고 이 줄에 머문다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Subtitle")
	float Duration = 3.f;

	/** 이 줄 다음 이어갈 행. None 이면 자막 종료. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Subtitle")
	FName NextRow;
};
