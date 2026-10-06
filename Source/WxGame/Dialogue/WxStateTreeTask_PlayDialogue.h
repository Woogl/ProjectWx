// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_PlayDialogue.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UWxDialogueSessionComponent;


USTRUCT()
struct FWxStateTreeTask_PlayDialogueInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (RowType = "/Script/WxGame.WxDialogueTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle StartRow;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWxDialogueSessionComponent> Session;

	FDelegateHandle EndedHandle;
	bool bNeedsRestart = false;
};

/**
 * 0번 컨트롤러(v1 싱글/리슨 호스트 전제)의 대화 세션에 대사를 열고, 마지막 행까지 읽었을 때만 Succeeded 로 마감해 읽지 않은 대사에 트리가 전진하지 않는다.
 *
 * 중단(대화 중 사망·행 해석 실패·다른 대화가 끼어듦)이면 살아 있는 폰과 빈 세션을 기다려 상태 재진입 없이 시작 행부터 다시 열어, 같은 상태의 보상을 중복 지급하지 않는다.
 *
 * 상태를 먼저 떠나도 대화를 끊지 않는다 — 읽던 대사가 사라지는 편이 더 나쁘다.
 */
USTRUCT(meta = (DisplayName = "Play Dialogue", Category = "Wx"))
struct FWxStateTreeTask_PlayDialogue : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlayDialogueInstanceData;

	FWxStateTreeTask_PlayDialogue();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

private:
	EStateTreeRunStatus StartDialogue(FStateTreeExecutionContext& Context) const;
};
