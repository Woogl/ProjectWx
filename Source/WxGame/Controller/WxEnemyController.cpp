// Copyright Woogle. All Rights Reserved.

#include "WxEnemyController.h"
#include "WxBlackboardKeys.h"
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
		WxBlackboardKeys::SetSelfActor(BB, InPawn);
		WxBlackboardKeys::SetHomeLocation(BB, InPawn->GetActorLocation());
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
		WxBlackboardKeys::SetSelfActor(BB, nullptr);
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
		WxBlackboardKeys::ClearPatrolTargetLocation(BB);
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
		WxBlackboardKeys::SetPatrolTargetLocation(BB, PatrolPoints[PatrolCursor]);
	}
}

bool AWxEnemyController::AdvanceToNextPatrolPoint()
{
	// 경로 없음: 향할 정찰 지점이 없다.
	if (PatrolPoints.Num() == 0)
	{
		return false;
	}

	// 단일 지점: 그 자리에 머문다(진행은 없지만 정찰은 계속 유효).
	if (PatrolPoints.Num() == 1)
	{
		return true;
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
				WxBlackboardKeys::ClearPatrolTargetLocation(BB);
			}
			return false;
		}
		PatrolCursor += 1;
		break;
	}

	PublishPatrolTarget();
	return true;
}
