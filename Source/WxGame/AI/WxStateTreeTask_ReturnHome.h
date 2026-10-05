// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeMoveToTask.h"
#include "WxStateTreeTask_ReturnHome.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

/**
 * 리시 이탈 전이로 들어오는 복귀 상태의 실행 노드다. 엔진 Move To 를 상속해 AWxAIController 의 홈 위치로 걸어간다.
 * 복귀가 언제 끝나는지는 이 태스크가 단독으로 정한다 — 복귀 상태에는 완료 전이만 두므로 폰이 반경 안으로 재진입해도 이동을 끊지 않는다.
 * 이동이 실제로 시작되면 겨누던 대상의 퍼셉션 기록을 지우고 대상을 비운다.
 * 이후 새 자극은 억제하지 않으므로 정상 감지 경로를 통해 다시 타겟을 획득할 수 있다.
 *
 * 이동 목표(Destination·TargetActor)는 진입 때 홈 위치로 덮어쓰므로 저작 대상이 아니다.
 */
USTRUCT(meta = (DisplayName = "홈 복귀", Category = "Wx"))
struct FWxStateTreeTask_ReturnHome : public FStateTreeMoveToTask
{
	GENERATED_BODY()

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
