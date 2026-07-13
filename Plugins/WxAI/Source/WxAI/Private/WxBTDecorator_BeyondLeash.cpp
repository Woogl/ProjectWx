// Copyright Woogle. All Rights Reserved.

#include "WxBTDecorator_BeyondLeash.h"

#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UWxBTDecorator_BeyondLeash::UWxBTDecorator_BeyondLeash()
{
	NodeName = TEXT("Beyond Leash");
}

FString UWxBTDecorator_BeyondLeash::GetStaticDescription() const
{
	return FString::Printf(TEXT("Home 에서 %.0f 이상 이탈 시 true"), LeashRadius);
}

bool UWxBTDecorator_BeyondLeash::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !Blackboard)
	{
		return false;
	}

	const float DistSquared = FVector::DistSquared(Pawn->GetActorLocation(), WxBlackboardKeys::GetHomeLocation(Blackboard));
	return DistSquared > LeashRadius * LeashRadius;
}
