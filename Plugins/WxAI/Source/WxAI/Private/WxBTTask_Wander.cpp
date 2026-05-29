// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_Wander.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_Wander::UWxBTTask_Wander()
{
	NodeName = TEXT("Wander");
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

	const float Yaw = FMath::FRandRange(0.f, 360.f);
	MoveDirection = FRotator(0.f, Yaw, 0.f).Vector();

	const float MinTime = FMath::Min(MinDuration, MaxDuration);
	const float MaxTime = FMath::Max(MinDuration, MaxDuration);
	TotalTime = FMath::FRandRange(MinTime, MaxTime);
	ElapsedTime = 0.f;

	return EBTNodeResult::InProgress;
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

FString UWxBTTask_Wander::GetStaticDescription() const
{
	return FString::Printf(TEXT("Wander: Duration=%.1f~%.1fs, Speed x%.2f"), MinDuration, MaxDuration, MoveSpeedMultiplier);
}
