// Copyright Woogle. All Rights Reserved.

#include "WxEnemyController.h"
#include "WxAIBlackboardKeys.h"
#include "WxAIPerceptionComponent.h"
#include "Character/WxEnemyCharacter.h"
#include "Spawnable/WxPatrolComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GenericTeamAgentInterface.h"

AWxEnemyController::AWxEnemyController()
{
	WxAIPerceptionComponent = CreateDefaultSubobject<UWxAIPerceptionComponent>(TEXT("WxAIPerceptionComponent"));

	PatrolMoveMode = EWxPatrolMoveMode::Loop;
}

void AWxEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (const IGenericTeamAgentInterface* PawnTeam = Cast<IGenericTeamAgentInterface>(InPawn))
	{
		SetGenericTeamId(PawnTeam->GetGenericTeamId());
	}

	// Blackboard 컴포넌트는 RunBehaviorTree 안에서 생성되므로, BT 를 먼저 실행한 뒤에 컨텍스트 키를 세팅한다.
	// (순서를 반대로 하면 GetBlackboardComponent() 가 null 이라 SelfActor/HomeLocation 세팅이 통째로 누락된다.)
	if (AWxEnemyCharacter* Enemy = Cast<AWxEnemyCharacter>(InPawn))
	{
		WxAIPerceptionComponent->ApplySenseSettings(Enemy->GetSightRadius(), Enemy->GetSightAngle(), Enemy->GetMaxHearingRange());

		if (UBehaviorTree* BT = Enemy->GetBehaviorTree())
		{
			RunBehaviorTree(BT);
		}
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		WxAIBlackboardKeys::SetSelfActor(BB, InPawn);
		WxAIBlackboardKeys::SetHomeLocation(BB, InPawn->GetActorLocation());
	}

	if (const AWxEnemyCharacter* Enemy = Cast<AWxEnemyCharacter>(InPawn))
	{
		InitializePatrol(Enemy);
	}
}

void AWxEnemyController::OnUnPossess()
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		WxAIBlackboardKeys::SetSelfActor(BB, nullptr);
	}

	PatrolPoints.Reset();

	Super::OnUnPossess();
}

void AWxEnemyController::InitializePatrol(const AWxEnemyCharacter* Enemy)
{
	PatrolPoints.Reset();
	PatrolCursor = 0;
	PatrolDirection = 1;
	PatrolMoveMode = EWxPatrolMoveMode::Loop;

	if (const UWxPatrolComponent* Patrol = Enemy->GetPatrolComponent())
	{
		PatrolPoints = Patrol->GetWorldPoints();
		PatrolMoveMode = Patrol->GetMoveMode();
	}

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	if (PatrolPoints.Num() > 0)
	{
		PublishPatrolTarget();
	}
	else
	{
		// 경로 없음: 목표를 비워 둔다. BT 정찰 서브트리는 PatrolTargetLocation 의 Set 여부로 게이트한다.
		WxAIBlackboardKeys::ClearPatrolTargetLocation(BB);
	}
}

void AWxEnemyController::PublishPatrolTarget()
{
	if (!PatrolPoints.IsValidIndex(PatrolCursor))
	{
		return;
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		WxAIBlackboardKeys::SetPatrolTargetLocation(BB, PatrolPoints[PatrolCursor]);
	}
}

bool AWxEnemyController::HasPatrolRoute() const
{
	return PatrolPoints.Num() > 0;
}

void AWxEnemyController::AdvanceToNextPatrolPoint()
{
	// 0개: 경로 없음. 1개: 진행할 다음 지점이 없음.
	if (PatrolPoints.Num() <= 1)
	{
		return;
	}

	switch (PatrolMoveMode)
	{
	case EWxPatrolMoveMode::Loop:
		PatrolCursor = (PatrolCursor + 1) % PatrolPoints.Num();
		break;

	case EWxPatrolMoveMode::PingPong:
		if (PatrolCursor + PatrolDirection < 0 || PatrolCursor + PatrolDirection >= PatrolPoints.Num())
		{
			PatrolDirection = -PatrolDirection;
		}
		PatrolCursor += PatrolDirection;
		break;

	case EWxPatrolMoveMode::Once:
		if (PatrolCursor + 1 >= PatrolPoints.Num())
		{
			// 마지막 지점 도달: 정찰 종료. 목표를 비우면 게이트(PatrolTargetLocation Is Set)가 닫혀 정찰 서브트리가 멈춘다.
			if (UBlackboardComponent* BB = GetBlackboardComponent())
			{
				WxAIBlackboardKeys::ClearPatrolTargetLocation(BB);
			}
			return;
		}
		PatrolCursor += 1;
		break;
	}

	PublishPatrolTarget();
}
