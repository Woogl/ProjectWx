// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_Wander.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "WxGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_Wander::UWxBTTask_Wander()
{
	bCreateNodeInstance = true;
	bNotifyTick = true;

	// 배회 종료(완료·중단·실패)에서 낮췄던 이동 속도를 복원하기 위해 종료 콜백을 받는다.
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

	// 8 = EWxWanderDirection 의 방향 개수.
	TArray<int32, TInlineAllocator<8>> AllowedIndices;
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
		const int32 ChosenIndex = AllowedIndices[FMath::RandRange(0, AllowedIndices.Num() - 1)];
		Yaw = AIController->GetControlRotation().Yaw + ChosenIndex * 45.f;
	}
	else
	{
		Yaw = FMath::FRandRange(0.f, 360.f);
	}
	MoveDirection = FRotator(0.f, Yaw, 0.f).Vector();

	TotalTime = Duration;
	ElapsedTime = 0.f;

	// 감속 GE 부여·제거는 Patrol 과 동일하다.
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
