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

	/** 대사 본문. 모든 행이 채워야 한다 — 종료는 NextRow=None 으로 표시하며, 비어 있으면 잘못된 행으로 보고 경고와 함께 대화를 접는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue", meta = (MultiLine = "true"))
	FText Line;
	
	/**
	 * 이 대사 동안 NPC가 취할 포즈. 비우면 직전 포즈를 그대로 둔다.
	 *
	 * 소프트 참조다 — 대화 테이블은 배치 NPC 가 하드로 붙잡아 레벨과 함께 상주하므로, 하드로 두면 말을 걸기 전부터 이 테이블 모든 행의 몽타주가 메모리에 올라온다.
	 * 세션이 대사를 넘길 때 비동기로 스트리밍한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	TSoftObjectPtr<UAnimMontage> TargetPose;

	/** 이 대사 다음 이어갈 행. None 이면 대화 종료. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FName NextRow;
};
