// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxDialogueTableRow.generated.h"

/** 선택지 하나. 고르면 같은 테이블의 TargetDialogue 노드로 점프한다. */
USTRUCT(BlueprintType)
struct FWxDialogueChoice
{
	GENERATED_BODY()

	/** 선택지 표시 텍스트. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FText ChoiceText;

	/** 선택 시 이어갈 노드(같은 테이블의 행 이름). None 이면 대화 종료. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FName TargetDialogue;
};

/**
 * 대화 노드 하나 = 대사 한 줄 = 행 하나. 대화 1편 = 테이블 1개.
 * 대사를 출력한 뒤, Choices 가 있으면 선택을 기다리고 없으면 NextDialogue 로 이어간다.
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

	/** 이 대사에서 제시할 선택지들. 비어 있으면 NextDialogue 선형 진행. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	TArray<FWxDialogueChoice> Choices;

	/** 이 대사 다음(선택지가 없을 때) 이어갈 노드. None 이면 대화 종료. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FName NextDialogue;
};
