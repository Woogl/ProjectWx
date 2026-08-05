// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxDialogueTableRow.generated.h"

class UAnimMontage;

/**
 * 대화 노드 하나 = 대사 한 줄 = 행 하나. 대화 1편 = 테이블 1개.
 * 대사를 출력한 뒤 NextRow 로 이어가고, 가리키는 곳이 없으면 대화가 끝난다.
 */
USTRUCT(BlueprintType)
struct FWxDialogueTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 화자 표시명. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FText Speaker;

	/** 대사 본문. 비어 있으면 대화가 종료된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue", meta = (MultiLine = "true"))
	FText Line;
	
	/**
	 * 이 대사 동안 NPC가 취할 포즈. 비우면 직전 포즈를 그대로 둔다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	TObjectPtr<UAnimMontage> TargetPose;

	/** 이 대사 다음 이어갈 행. None 이면 대화 종료. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FName NextRow;
};
