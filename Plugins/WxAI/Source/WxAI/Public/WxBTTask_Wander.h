// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WxBTTask_Wander.generated.h"

/** 배회 이동 방향. 폰의 정면(ControlRotation)을 기준으로 시계 방향 45도 간격의 8방향. */
UENUM()
enum class EWxWanderDirection : uint8
{
	Forward,
	ForwardRight,
	Right,
	BackRight,
	Back,
	BackLeft,
	Left,
	ForwardLeft,
};

/**
 * BT Task: 지정한 방향들 중 하나로 일정 시간 동안 이동한다.
 *
 * 타겟을 바라본 채 이동(strafe)할지는 이 태스크가 아니라 타겟의 원천인 UWxAIPerceptionComponent 가 회전 모드를 발행해 결정한다.
 */
UCLASS()
class WXAI_API UWxBTTask_Wander : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UWxBTTask_Wander();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 총 배회 지속 시간(초) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Duration = 1.f;

	/** 이동 가능한 방향(폰 정면 기준 8방향, 복수 선택). 매 실행마다 선택된 방향 중 하나를 무작위로 고른다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (Bitmask, BitmaskEnum = "/Script/WxAI.EWxWanderDirection"))
	int32 Directions = 0xFF;

	/** 배회 중 이동 입력 배율. 최대 이동 속도가 이 비율로 제한된다. (1.0 = 평상시 속도) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MoveSpeedMultiplier = 0.5f;

private:
	FVector MoveDirection = FVector::ForwardVector;

	float TotalTime = 0.f;

	float ElapsedTime = 0.f;
};
