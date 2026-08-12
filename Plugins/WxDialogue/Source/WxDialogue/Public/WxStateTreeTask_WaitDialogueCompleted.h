// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_WaitDialogueCompleted.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_WaitDialogueCompletedInstanceData
{
	GENERATED_BODY()

	/**
	 * 완주를 판정할 대화의 노드. 대화 안의 어느 행이든 지목할 수 있다.
	 * 시작 행을 넣으면 그 대화 전체가, 중간·끝 행을 넣으면 그 대사까지 읽은 대화만 게이트를 통과한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (RowType = "/Script/WxDialogue.WxDialogueTableRow"))
	FDataTableRowHandle DialogueRow;

	/** (런타임) 이 상태에 머무는 동안 지정 대사를 목격했는가. 목격 후 대화가 끝나면 완주다. */
	UPROPERTY()
	bool bObservedDialogue = false;
};

/**
 * 로컬 플레이어(0번 컨트롤러)의 대화 세션을 관찰해, 이 상태에 머무는 동안 지정 대사를 거친 대화가 끝나면 Succeeded 로 완료한다(수주·납품 게이트).
 * 플레이어가 스스로 건 대화를 관찰만 한다 — 트리가 대사를 여는 쪽은 'Play Dialogue' 다.
 * 목격은 진행 중인 대사와 지정 행을 맞춰 보고, 완주는 그 뒤 대화창이 닫히는 것으로 본다.
 * 같은 NPC 라도 퀘스트 단계마다 다른 대사를 게이트로 삼을 수 있고, 대상 없는 대사(나레이션)도 같은 방식으로 판정된다.
 * 진입 이전의 대화는 세지 않는다 — 세션의 현재 대사만 관찰하는 엣지 감지라 기록이 필요 없고, 퀘스트 재탑재 시에도 게이트가 새 대화를 요구하게 된다(즉시 통과 없음).
 * 중간 행은 플레이어가 다음 대사로 넘기기 전까지만 현재라 폴링으로 본다 — 자동 넘김이 없어 한 대사가 여러 틱 머무는 지금은 놓칠 일이 없다.
 * 대화는 뜻을 해석하지 않으므로, "이 대화가 수주다" 같은 의미 판정은 이 태스크를 놓는 퀘스트 데이터의 몫이다.
 * 행 미지정은 완료될 수 없는 잘못된 조립이므로 진입 시 경고를 남긴다.
 * 0번 컨트롤러 사용은 다른 크로스모듈 노드와 같은 전제(v1 싱글/리슨 호스트)다.
 */
USTRUCT(meta = (DisplayName = "Wait Dialogue Completed", Category = "Wx"))
struct FWxStateTreeTask_WaitDialogueCompleted : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_WaitDialogueCompletedInstanceData;

	FWxStateTreeTask_WaitDialogueCompleted();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
