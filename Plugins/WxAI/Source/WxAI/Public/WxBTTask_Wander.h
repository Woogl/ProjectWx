// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WxBTTask_Wander.generated.h"

/**
 * BT Task: 랜덤한 방향으로 일정 시간 동안 이동한다.
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
	
	/** 총 배회 지속 시간 최소값(초) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinDuration = 1.f;

	/** 총 배회 지속 시간 최대값(초) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDuration = 1.5f;

	/** 배회 중 이동 입력 배율. 최대 이동 속도가 이 비율로 제한된다. (1.0 = 평상시 속도) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MoveSpeedMultiplier = 0.5f;

private:
	FVector MoveDirection = FVector::ForwardVector;

	float TotalTime = 0.f;

	float ElapsedTime = 0.f;
};
