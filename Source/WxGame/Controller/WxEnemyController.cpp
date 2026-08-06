// Copyright Woogle. All Rights Reserved.

#include "WxEnemyController.h"
#include "WxBlackboardKeys.h"
#include "WxAIPerceptionComponent.h"
#include "Character/WxEnemyCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
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

	// Blackboard 컴포넌트는 RunBehaviorTree 안에서 생성되므로, BT 를 먼저 실행한 뒤에 컨텍스트 키를 세팅한다.
	// (순서를 반대로 하면 GetBlackboardComponent() 가 null 이라 SelfActor/HomeLocation 세팅이 통째로 누락된다.)
	if (AWxEnemyCharacter* Enemy = Cast<AWxEnemyCharacter>(InPawn))
	{
		Enemy->OnDeath.AddDynamic(this, &AWxEnemyController::HandlePawnDeath);

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
}

void AWxEnemyController::OnUnPossess()
{
	// 엔진이 Super 에서 폰 참조를 끊으므로, 그보다 앞에서 이전 폰을 찾아 구독을 해제한다.
	if (AWxEnemyCharacter* Enemy = Cast<AWxEnemyCharacter>(GetPawn()))
	{
		Enemy->OnDeath.RemoveDynamic(this, &AWxEnemyController::HandlePawnDeath);
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		WxBlackboardKeys::SetSelfActor(BB, nullptr);
	}

	Super::OnUnPossess();
}

void AWxEnemyController::HandlePawnDeath(AWxCharacterBase* DeadCharacter)
{
	// 사망하면 어빌리티는 State.Dead 에 막히고 이동은 래그돌이 막으므로, 트리를 계속 돌리면 실패할 브랜치만 매 틱 다시 고른다.
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Pawn died"));
	}
}
