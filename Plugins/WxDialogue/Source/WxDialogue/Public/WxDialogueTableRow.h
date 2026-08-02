// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WxDialogueTableRow.generated.h"

class UAnimMontage;

/**
 * 대화 노드 하나 = 대사 한 줄 = 행 하나. 대화 1편 = 테이블 1개.
 * 대사를 출력한 뒤 NextDialogue 로 이어가고, 가리키는 곳이 없으면 대화가 끝난다.
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
	 * 자기 자신으로 이어지는 섹션을 가진 몽타주는 다음 포즈가 올 때까지 자세를 붙잡고(대화가 끝나도 그대로 남는다), 루프가 없으면 한 번 연기한 뒤 기본 자세로 풀린다 — 유지·일회성은 애셋이 정한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	TObjectPtr<UAnimMontage> TargetPose;

	/** 이 대사 다음 이어갈 노드. None 이면 대화 종료. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Dialogue")
	FName NextDialogue;
};
