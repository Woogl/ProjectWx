// Copyright Woogle. All Rights Reserved.

#include "WxEnemyController.h"
#include "WxAIBlackboardKeys.h"
#include "WxAIPerceptionComponent.h"
#include "Character/WxEnemyCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GenericTeamAgentInterface.h"

AWxEnemyController::AWxEnemyController()
{
	WxAIPerceptionComponent = CreateDefaultSubobject<UWxAIPerceptionComponent>(TEXT("WxAIPerceptionComponent"));
}

void AWxEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (const IGenericTeamAgentInterface* PawnTeam = Cast<IGenericTeamAgentInterface>(InPawn))
	{
		SetGenericTeamId(PawnTeam->GetGenericTeamId());
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(WxAIBlackboardKeys::SelfActor, InPawn);
		if (InPawn)
		{
			BB->SetValueAsVector(WxAIBlackboardKeys::HomeLocation, InPawn->GetActorLocation());
		}
	}

	if (AWxEnemyCharacter* Enemy = Cast<AWxEnemyCharacter>(InPawn))
	{
		if (UBehaviorTree* BT = Enemy->GetBehaviorTree())
		{
			RunBehaviorTree(BT);
		}
	}
}

void AWxEnemyController::OnUnPossess()
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(WxAIBlackboardKeys::SelfActor, nullptr);
	}

	Super::OnUnPossess();
}
