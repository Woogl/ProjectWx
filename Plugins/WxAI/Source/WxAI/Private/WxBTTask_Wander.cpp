// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_Wander.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_Wander::UWxBTTask_Wander()
{
	bCreateNodeInstance = true;
	bNotifyTick = true;
}

EBTNodeResult::Type UWxBTTask_Wander::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn())
	{
		return EBTNodeResult::Failed;
	}

	// 선택된 방향들을 모아 그중 하나를 무작위로 고른다. 8 = EWxWanderDirection 의 방향 개수.
	TArray<int32> AllowedIndices;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		if (Directions & (1 << Index))
		{
			AllowedIndices.Add(Index);
		}
	}

	float Yaw;
	if (AllowedIndices.Num() > 0)
	{
		// 폰 정면(ControlRotation) 기준 시계 방향 45도 간격. 45 = 360 / 8방향.
		const int32 ChosenIndex = AllowedIndices[FMath::RandRange(0, AllowedIndices.Num() - 1)];
		Yaw = AIController->GetControlRotation().Yaw + ChosenIndex * 45.f;
	}
	else
	{
		// 아무 방향도 선택되지 않았으면 완전 무작위 방향으로 폴백한다.
		Yaw = FMath::FRandRange(0.f, 360.f);
	}
	MoveDirection = FRotator(0.f, Yaw, 0.f).Vector();

	TotalTime = Duration;
	ElapsedTime = 0.f;

	return EBTNodeResult::InProgress;
}

FString UWxBTTask_Wander::GetStaticDescription() const
{
	return FString::Printf(TEXT("Duration: %.1f s\nSpeed: x %.1f"), Duration, MoveSpeedMultiplier);
}

void UWxBTTask_Wander::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	ElapsedTime += DeltaSeconds;
	if (ElapsedTime >= TotalTime)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (APawn* Pawn = AIController ? AIController->GetPawn() : nullptr)
	{
		Pawn->AddMovementInput(MoveDirection, MoveSpeedMultiplier);
	}
}
