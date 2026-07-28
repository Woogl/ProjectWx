// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StateTreeTaskBase.h"
#include "WxDialogueStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

/**
 * 대화를 StateTree 에서 출력·판정하는 노드.
 * 대화 시스템을 소유한 본 모듈이 노드까지 함께 제공한다 — 보상 지급 노드를 WxInventory 가, 인디케이터 노드를 WxUI 가 소유하는 것과 같은 모양이라,
 * 퀘스트 같은 소비 도메인이 대화 모듈을 참조하지 않고도 에셋에서 이 노드를 골라 쓸 수 있다.
 *
 * 두 노드 모두 대화를 시작 행으로 지목한다 — 대화 정의 컴포넌트가 쓰는 것과 같은 값이라, 관찰(대기)과 출력이 같은 어휘로 맞물린다.
 *
 *  - WaitDialogueCompleted 는 플레이어가 스스로 건 대화를 관찰만 한다(수주·납품 게이트).
 *  - PlayDialogue 는 반대로 트리가 대사를 열어 연출한다(독백·무전·처치 후 대사).
 */

// ── WaitDialogueCompleted: 지정 대화의 완주 대기 ──────────────────────────────

USTRUCT()
struct FWxStateTreeTask_WaitDialogueCompletedInstanceData
{
	GENERATED_BODY()

	/** 완주를 판정할 대화의 시작 노드. 대상 액터의 대화 정의나 Play Dialogue 가 이 행에서 연 대화만 센다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (RowType = "/Script/WxDialogue.WxDialogueTableRow"))
	FDataTableRowHandle StartRow;

	/** (런타임) 이 상태에 머무는 동안 지정 대화를 목격했는가. 목격 후 대화가 끝나면 완주다. */
	UPROPERTY()
	bool bObservedDialogue = false;
};

/**
 * 로컬 플레이어(0번 컨트롤러)의 대화 세션을 관찰해, 이 상태에 머무는 동안 지정 대화가 시작되고 끝나면 Succeeded 로 완료한다.
 * 대화의 신원은 세션을 연 시작 행으로 가린다 — 세션이 열려 있는 동안 변하지 않는 값이라 틱 샘플링으로 놓치지 않고,
 * 같은 NPC 라도 퀘스트 단계마다 다른 대사를 게이트로 삼을 수 있다. 대상 없는 대사(나레이션)도 같은 방식으로 판정된다.
 * 진입 이전의 대화는 세지 않는다 — 세션의 현재 대화만 관찰하는 엣지 감지라 기록이 필요 없고,
 * 퀘스트 재탑재 시에도 게이트가 새 대화를 요구하게 된다(즉시 통과 없음).
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

// ── PlayDialogue: 트리가 여는 대화 ────────────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_PlayDialogueInstanceData
{
	GENERATED_BODY()

	/** 출력할 대화의 시작 노드. 이후 진행(다음 행·선택지)은 대화 데이터가 정한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (RowType = "/Script/WxDialogue.WxDialogueTableRow"))
	FDataTableRowHandle StartRow;
};

/**
 * 진입 시 로컬 플레이어(0번 컨트롤러)의 대화 세션에 지정 대사를 열고, 그 대화가 끝날 때까지 Running 으로 머물다 종료되면 Succeeded 로 완료한다.
 * 대사를 트리가 소유하므로(대상 액터의 대화 정의가 아니라) 같은 NPC 라도 퀘스트 단계마다 다른 대사를 낼 수 있고, 화자 없는 독백도 낼 수 있다.
 * 대화 대상을 두지 않으므로 카메라는 플레이어에 머문다 — 특정 액터를 비추는 연출이 필요해지면 별도 태스크로 분리한다.
 * 세션 부재·StartRow 미지정·행 해석 실패(행 없음·대사 빔)는 완주할 수 없는 잘못된 조립이므로 경고를 남기고 Failed.
 * 상태를 먼저 떠나도 대화를 끊지 않는다 — 읽던 대사가 사라지는 편이 더 나쁘고, 세션은 자기 데이터를 끝까지 진행한다.
 * 종료 판정은 권위 측에서 세션 상태를 폴링한다. 0번 컨트롤러 사용과 함께 다른 크로스모듈 노드와 같은 전제(v1 싱글/리슨 호스트)다.
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

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
