// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_Wander.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_Wander::UWxBTTask_Wander()
{
	bCreateNodeInstance = true;
	bNotifyTick = true;

	// 배회 종료(도착·중단·실패)에서 낮췄던 이동 속도를 복원하기 위해 종료 콜백을 받는다.
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UWxBTTask_Wander::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
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

	// 배회 이동 동안만 폰의 최대 이동 속도를 배율만큼 낮춘다. 원래 값은 OnTaskFinished 에서 복원한다(Patrol 과 동일).
	CachedMaxWalkSpeed = 0.f;
	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			CachedMaxWalkSpeed = Movement->MaxWalkSpeed;
			Movement->MaxWalkSpeed *= MoveSpeedMultiplier;
		}
	}

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
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 속도는 위에서 낮춘 MaxWalkSpeed 가 제어하므로 입력 스케일은 1.0 으로 넣는다.
	Pawn->AddMovementInput(MoveDirection, 1.f);
}

void UWxBTTask_Wander::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	// 이동 동안 낮췄던 최대 이동 속도를 복원한다. 도착·중단·실패 등 어떤 종료 경로에서도 호출된다(Patrol 과 동일).
	if (CachedMaxWalkSpeed > 0.f)
	{
		const AAIController* AIController = OwnerComp.GetAIOwner();
		const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
		if (const ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = CachedMaxWalkSpeed;
			}
		}
		CachedMaxWalkSpeed = 0.f;
	}
}
