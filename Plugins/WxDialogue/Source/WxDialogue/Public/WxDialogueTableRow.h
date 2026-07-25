// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxDialogueTableRow.generated.h"

/** 대사 한 줄. */
USTRUCT(BlueprintType)
struct FWxDialogueLine
{
	GENERATED_BODY()

	/** 화자 표시명. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FText Speaker;

	/** 대사 본문. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue", meta = (MultiLine = "true"))
	FText Text;
};

/** 선택지 하나. 고르면 같은 테이블의 TargetRow 노드로 점프한다. */
USTRUCT(BlueprintType)
struct FWxDialogueChoice
{
	GENERATED_BODY()

	/** 선택지 표시 텍스트. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FText ChoiceText;

	/** 선택 시 이어갈 노드(같은 테이블의 행 이름). None 이면 대화 종료. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FName TargetRow;
};

/**
 * 대화 노드 하나 = 행 하나. 대화 1편 = 테이블 1개.
 * Lines 를 순서대로 출력한 뒤, Choices 가 있으면 선택을 기다리고 없으면 NextRow 로 이어간다.
 */
USTRUCT(BlueprintType)
struct FWxDialogueTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 이 노드에서 순서대로 출력할 대사들. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	TArray<FWxDialogueLine> Lines;

	/** 마지막 대사에서 제시할 선택지들. 비어 있으면 NextRow 선형 진행. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	TArray<FWxDialogueChoice> Choices;

	/** Lines 소진 후(선택지가 없을 때) 이어갈 노드. None 이면 대화 종료. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FName NextRow;
};
