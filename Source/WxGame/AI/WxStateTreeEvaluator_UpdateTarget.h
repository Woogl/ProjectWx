// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "WxStateTreeEvaluator_UpdateTarget.generated.h"

struct FStateTreeExecutionContext;
class AActor;
class UAIPerceptionComponent;


USTRUCT()
struct FWxStateTreeEvaluator_UpdateTargetInstanceData
{
	GENERATED_BODY()

	float TimeUntilUpdate = 0.f;
};

/**
 * 퍼셉션이 감지한 액터 중 하나를 AWxAIController 의 겨누는 대상으로 올린다. 트리와 링크된 패턴 트리는 그 대상을 컨트롤러에서 읽는다.
 *
 * 타겟은 죽거나 사라지거나 어그로 비허용으로 바뀌면 갈린다. 시야에서 벗어나도 유지하며, 리시 복귀는 감지 기록을 지워 해제한다.
 */
USTRUCT(meta = (DisplayName = "Update Target", Category = "Wx"))
struct FWxStateTreeEvaluator_UpdateTarget : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeEvaluator_UpdateTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", ForceUnits = "s"))
	float Interval = 0.1f;

private:
	/**
	 * 세 센스 모두 적대만 등록하므로 피아는 다시 가르지 않는다.
	 * 겨누는 대상을 채우는 유일한 통로다 — 여기서 거른 대상은 다시 감지되어도 어그로가 되지 않는다.
	 */
	AActor* FindPerceivedTarget(const UAIPerceptionComponent& Perception, const AActor* SelfActor) const;
};
