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
	WxAIPerceptionComponent->OnTargetChanged.AddUObject(this, &ThisClass::HandleAITargetChanged);
}

void AWxEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (const IGenericTeamAgentInterface* PawnTeam = Cast<IGenericTeamAgentInterface>(InPawn))
	{
		SetGenericTeamId(PawnTeam->GetGenericTeamId());

		// 퍼셉션 리스너는 빙의 전에 등록되면서 그 자리에서 컨트롤러 팀을 캐시하는데, 엔진은 팀이 바뀌어도 퍼셉션에 통보하지 않는다.
		// 청각·촉각의 피아 판정이 그 캐시를 쓰므로, 여기서 갱신하지 않으면 무팀으로 남아 자기 발소리와 아군 소음까지 적대로 듣는다.
		WxAIPerceptionComponent->RequestStimuliListenerUpdate();
	}

	// Blackboard 컴포넌트는 RunBehaviorTree 안에서 생성되므로, BT 를 먼저 실행한 뒤에 컨텍스트 키를 세팅한다.
	if (AWxEnemyCharacter* Enemy = Cast<AWxEnemyCharacter>(InPawn))
	{
		// 재사용된 폰도 새 빙의에서는 타겟 없이 시작한다.
		Enemy->SetHasAITarget(false);
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
		Enemy->SetHasAITarget(false);
		Enemy->OnDeath.RemoveDynamic(this, &AWxEnemyController::HandlePawnDeath);
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		WxBlackboardKeys::SetSelfActor(BB, nullptr);
	}

	Super::OnUnPossess();
}

void AWxEnemyController::HandleAITargetChanged(AActor* NewTarget)
{
	if (AWxEnemyCharacter* Enemy = Cast<AWxEnemyCharacter>(GetPawn()))
	{
		Enemy->SetHasAITarget(NewTarget != nullptr);
	}
}

void AWxEnemyController::HandlePawnDeath(AWxCharacterBase* DeadCharacter)
{
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Pawn died"));
	}
}
