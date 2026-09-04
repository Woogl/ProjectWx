// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "WxDialogueComponent.generated.h"

/**
 * 어느 노드에서 대화를 시작할지만 보유하고, 세션 진행은 상호작용한 플레이어 측(UWxDialogueSessionComponent)이 소유한다.
 *
 * 상호작용 계약은 호스트 액터(AWxDialogueActor)가 들고 이 컴포넌트로 넘긴다.
 * 그래서 이 컴포넌트를 아무 액터에 붙여도 말을 걸 수 있게 되지는 않는다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXDIALOGUE_API UWxDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	const FDataTableRowHandle& GetStartRow() const;

	FText GetTalkPrompt() const;

	/** 주체의 컨트롤러에서 세션을 찾아 대화를 연다. 서버 권위에서 호스트 액터의 상호작용 응답이 부른다. */
	void StartDialogueWith(AActor* Interactor);

protected:
	/** 비우면 대화가 시작되지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Dialogue", meta = (RowType = "/Script/WxDialogue.WxDialogueTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle StartRow;

	/** 상호작용 프롬프트에 쓰인다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Dialogue")
	FText SpeakerName;
};
