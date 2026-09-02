// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_Wander.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "WxGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_Wander::UWxBTTask_Wander()
{
	bCreateNodeInstance = true;
	bNotifyTick = true;

	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UWxBTTask_Wander::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	// 8 = EWxWanderDirection 의 방향 개수.
	TArray<int32, TInlineAllocator<8>> RemainingIndices;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		if (Directions & (1 << Index))
		{
			RemainingIndices.Add(Index);
		}
	}

	// 검증 거리는 이번 실행에서 실제로 걸어갈 거리다.
	// 감속 GE 는 방향을 고른 뒤 부여되므로 지금 최대 속도는 아직 평상시 값이고, GE 미지정이면 배율 자체가 걸리지 않는다.
	const UPawnMovementComponent* Movement = Pawn->GetMovementComponent();
	const float TravelDistance = Movement ? Movement->GetMaxSpeed() * (MoveSpeedEffect ? MoveSpeedMultiplier : 1.f) * Duration : 0.f;
	const FVector NavStart = Pawn->GetNavAgentLocation();

	bool bFoundDirection = false;
	while (RemainingIndices.Num() > 0)
	{
		const int32 PickedSlot = FMath::RandRange(0, RemainingIndices.Num() - 1);
		const FVector Candidate = FRotator(0.f, AIController->GetControlRotation().Yaw + RemainingIndices[PickedSlot] * 45.f, 0.f).Vector();
		RemainingIndices.RemoveAtSwap(PickedSlot);

		// 내비 데이터가 아예 없어도 막힘으로 온다 — 내비메시 없는 맵에서는 배회하지 않는다.
		FVector HitLocation;
		if (UNavigationSystemV1::NavigationRaycast(Pawn, NavStart, NavStart + Candidate * TravelDistance, HitLocation, nullptr, AIController))
		{
			continue;
		}

		MoveDirection = Candidate;
		bFoundDirection = true;
		break;
	}

	// 허용 방향이 하나도 없거나 전부 내비메시를 벗어난다. 상위 폴백 분기에 넘긴다.
	if (!bFoundDirection)
	{
		return EBTNodeResult::Failed;
	}

	ElapsedTime = 0.f;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (ASC && MoveSpeedEffect)
	{
		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(MoveSpeedEffect, 1.f, ASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_MoveSpeedScale, MoveSpeedMultiplier);
			MoveSpeedEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		}
	}

	return EBTNodeResult::InProgress;
}

FString UWxBTTask_Wander::GetStaticDescription() const
{
	if (!MoveSpeedEffect)
	{
		return FString::Printf(TEXT("Duration: %.1f s\nSpeed: 감속 GE 미지정"), Duration);
	}

	return FString::Printf(TEXT("Duration: %.1f s\nSpeed: x %.1f"), Duration, MoveSpeedMultiplier);
}

void UWxBTTask_Wander::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	ElapsedTime += DeltaSeconds;
	if (ElapsedTime >= Duration)
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

	// 속도는 감속 GE 가 낮춘 SPD → MaxWalkSpeed 가 제어하므로 입력 스케일은 1.0 으로 넣는다.
	Pawn->AddMovementInput(MoveDirection, 1.f);
}

void UWxBTTask_Wander::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	// 완료·중단·실패 등 어떤 종료 경로에서도 호출되므로, 감속 GE 제거는 여기서 한다.
	if (UAbilitySystemComponent* ASC = MoveSpeedEffectHandle.GetOwningAbilitySystemComponent())
	{
		ASC->RemoveActiveGameplayEffect(MoveSpeedEffectHandle);
	}
	MoveSpeedEffectHandle = FActiveGameplayEffectHandle();
}
