// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxActorTarget.h"
#include "WxDialogueStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

/**
 * 대화 사실을 StateTree 에서 판정하는 노드.
 * 대화 시스템을 소유한 본 모듈이 노드까지 함께 제공한다 — 보상 지급 노드를 WxInventory 가, 인디케이터 노드를 WxUI 가 소유하는 것과 같은 모양이라,
 * 퀘스트 같은 소비 도메인이 대화 모듈을 참조하지 않고도 에셋에서 이 노드를 골라 쓸 수 있다.
 *
 * 레벨 액터 지정은 FWxActorTarget(WxCore) 래퍼의 FUniversalObjectLocator 로 배치 액터를 직접 지정한다(스포너·인디케이터 노드와 동일).
 */

// ── WaitDialogueCompleted: 대상과의 대화 완주 대기 ────────────────────────────

USTRUCT()
struct FWxStateTreeTask_WaitDialogueCompletedInstanceData
{
	GENERATED_BODY()

	/** 완주를 판정할 대화 대상 배치 액터 지정. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FWxActorTarget Target;

	/** (런타임) 이 상태에 머무는 동안 대상과의 대화를 목격했는가. 목격 후 대화가 끝나면 완주다. */
	UPROPERTY()
	bool bObservedDialogue = false;
};

/**
 * 로컬 플레이어(0번 컨트롤러)의 대화 세션을 관찰해, 이 상태에 머무는 동안 지정 대상과의 대화가 시작되고 끝나면 Succeeded 로 완료한다.
 * 진입 이전의 대화는 세지 않는다 — 세션의 현재 대상만 관찰하는 엣지 감지라 기록이 필요 없고,
 * 퀘스트 재탑재 시에도 게이트가 새 대화를 요구하게 된다(즉시 통과 없음).
 * 대화는 뜻을 해석하지 않으므로, "이 대화가 수주다" 같은 의미 판정은 이 태스크를 놓는 퀘스트 데이터의 몫이다.
 * 빈 로케이터는 완료될 수 없는 잘못된 조립이므로 진입 시 경고를 남긴다.
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
