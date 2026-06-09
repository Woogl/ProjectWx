// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "WxBTTask_Patrol.generated.h"

/**
 * BT Task: 현재 정찰 지점으로 이동하고, 도착하면 커서를 다음 지점으로 진행시킨다.
 *
 * UBTTask_MoveTo 를 상속해 이동/도착 판정/경로 실패·중단 처리는 엔진에 맡기고, 도착(성공)했을 때만 정찰 커서를 한 칸 진행한다.
 * 이동 목표는 Blackboard 의 PatrolTargetLocation(BlackboardKey)에서 읽고, 도착 시 폰의 UWxPatrolComponent(FindPatrolComponent 로 조회)에 커서 진행을 위임한다.
 * 보통 정찰 시퀀스에 [Patrol -> Wait] 형태로 배치해, 한 지점 도착·대기 후 다음 지점으로 넘긴다.
 */
UCLASS()
class WXAI_API UWxBTTask_Patrol : public UBTTask_MoveTo
{
	GENERATED_BODY()

public:
	UWxBTTask_Patrol();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
	/** 정찰 중 이동 속도 배율. 최대 이동 속도가 이 비율로 제한된다. (1.0 = 평상시 속도) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MoveSpeedMultiplier = 0.5f;

	// 아래 상태들은 노드 인스턴스(폰)별로 보관된다(bCreateNodeInstance). 폰마다 독립된 커서를 가지므로 같은 경로 공유·리스폰에 안전하다.

	/** 현재 향하는 정찰 지점 인덱스. */
	int32 PatrolCursor = 0;

	/** PingPong 진행 방향(+1/-1). */
	int32 PatrolDirection = 1;

	/** Once 로 경로 끝에 도달해 정찰이 끝났는지. */
	bool bPatrolFinished = false;

	/** ExecuteTask 에서 낮추기 전의 MaxWalkSpeed. OnTaskFinished 에서 이 값으로 복원한다. */
	float CachedMaxWalkSpeed = 0.f;
};
