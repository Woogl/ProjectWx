// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_Patrol.h"

#include "WxPatrolComponent.h"
#include "AIController.h"
#include "WxBlackboardKeys.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UWxBTTask_Patrol::UWxBTTask_Patrol()
{
	NodeName = TEXT("Patrol");

	// 기본 이동 목표 키를 PatrolTargetLocation 으로 지정한다(에디터에서 별도 선택 없이 동작). 실제 키 해석은 InitializeFromAsset 가 한다.
	BlackboardKey.SelectedKeyName = WxBlackboardKeys::PatrolTargetLocation;

	// 도착 후 커서를 진행시키기 위해 종료 콜백을 받는다.
	bNotifyTaskFinished = true;

	// 정찰 커서·이동 속도 캐시를 폰별로 보관하기 위해 노드를 인스턴싱한다.
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UWxBTTask_Patrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;

	// 정찰 경로가 없는 적(정찰 안 함)은 실패시켜 Selector 가 다음 행동(배회 등)으로 넘어가게 한다.
	const UWxPatrolComponent* Patrol = UWxPatrolComponent::FindFor(Pawn);
	if (!Patrol || Patrol->GetNumPoints() == 0)
	{
		return EBTNodeResult::Failed;
	}

	// Once 로 경로를 마쳤으면 마지막 지점에 그대로 정지한다. Failed 를 반환하면 하위 폴백 분기가 폰을 집/배회로 끌고 가므로,
	// 이동 없이 Succeeded 로 정찰 분기를 점유해 그 자리에 머물게 한다.
	if (bPatrolFinished)
	{
		return EBTNodeResult::Succeeded;
	}

	// 현재 정찰 지점을 MoveTo 목표(PatrolTargetLocation)로 발행한 뒤 엔진 MoveTo 에 위임한다.
	if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		WxBlackboardKeys::SetPatrolTargetLocation(Blackboard, Patrol->GetPointLocation(PatrolCursor));
	}

	// 정찰 이동 동안만 폰의 최대 이동 속도를 배율만큼 낮춘다. 원래 값은 OnTaskFinished 에서 복원한다.
	CachedMaxWalkSpeed = 0.f;
	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			CachedMaxWalkSpeed = Movement->MaxWalkSpeed;
			Movement->MaxWalkSpeed *= MoveSpeedMultiplier;
		}
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

FString UWxBTTask_Patrol::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n도착 후 다음 정찰 지점으로 진행\nSpeed: x%.2f"), *Super::GetStaticDescription(), MoveSpeedMultiplier);
}

void UWxBTTask_Patrol::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;

	// 이동 동안 낮췄던 최대 이동 속도를 복원한다. 도착·중단·실패 등 어떤 종료 경로에서도 호출된다.
	if (CachedMaxWalkSpeed > 0.f)
	{
		if (const ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = CachedMaxWalkSpeed;
			}
		}
		CachedMaxWalkSpeed = 0.f;
	}

	// 도착(성공)했을 때만 커서를 다음 지점으로 진행한다. 중단·실패 시엔 커서를 보존해 재개 시 이어서 정찰한다.
	if (TaskResult != EBTNodeResult::Succeeded)
	{
		return;
	}

	if (const UWxPatrolComponent* Patrol = UWxPatrolComponent::FindFor(Pawn))
	{
		int32 NextIndex = PatrolCursor;
		if (Patrol->GetNextIndex(PatrolCursor, PatrolDirection, NextIndex))
		{
			PatrolCursor = NextIndex;
		}
		else
		{
			// Once 로 경로 끝에 도달: 이후 ExecuteTask 가 마지막 지점에 정지(Succeeded)하도록 표시한다.
			bPatrolFinished = true;
		}
	}
}
