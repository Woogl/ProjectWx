// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "WxDialogueComponent.generated.h"

/**
 * 대화 정의 컴포넌트. 대화를 걸 수 있는 액터(NPC, 기믹 등)에 붙는다.
 * 어느 노드에서 대화를 시작할지만 보유하고, 세션 진행은 상호작용한 플레이어 측(UWxDialogueSessionComponent)이 소유한다.
 * 소유 액터가 자기 상호작용 응답에서 상호작용자의 세션에 본 컴포넌트를 넘기면 대화가 열린다.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class WXDIALOGUE_API UWxDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	const FDataTableRowHandle& GetStartRow() const;

protected:
	/** 대화를 시작할 노드. 비우면 대화가 시작되지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Dialogue", meta = (RowType = "/Script/WxDialogue.WxDialogueTableRow"))
	FDataTableRowHandle StartRow;
};
