// Copyright Woogle. All Rights Reserved.

#include "WxAIBlackboardKeys.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

namespace WxAIBlackboardKeys
{
	const FName SelfActor = TEXT("SelfActor");

	const FName TargetActor = TEXT("TargetActor");

	const FName HomeLocation = TEXT("HomeLocation");

	const FName TargetLastKnownLocation = TEXT("TargetLastKnownLocation");

	const FName Phase = TEXT("Phase");

	const FName PatrolTargetLocation = TEXT("PatrolTargetLocation");

	AActor* GetTargetActor(const UBlackboardComponent* Blackboard)
	{
		return Cast<AActor>(Blackboard->GetValueAsObject(TargetActor));
	}

	void SetTargetActor(UBlackboardComponent* Blackboard, AActor* Value)
	{
		Blackboard->SetValueAsObject(TargetActor, Value);
	}

	void SetTargetLastKnownLocation(UBlackboardComponent* Blackboard, const FVector& Value)
	{
		Blackboard->SetValueAsVector(TargetLastKnownLocation, Value);
	}

	void ClearTargetLastKnownLocation(UBlackboardComponent* Blackboard)
	{
		Blackboard->ClearValue(TargetLastKnownLocation);
	}

	FVector GetHomeLocation(const UBlackboardComponent* Blackboard)
	{
		return Blackboard->GetValueAsVector(HomeLocation);
	}

	void SetHomeLocation(UBlackboardComponent* Blackboard, const FVector& Value)
	{
		Blackboard->SetValueAsVector(HomeLocation, Value);
	}

	void SetSelfActor(UBlackboardComponent* Blackboard, AActor* Value)
	{
		Blackboard->SetValueAsObject(SelfActor, Value);
	}

	void SetPatrolTargetLocation(UBlackboardComponent* Blackboard, const FVector& Value)
	{
		Blackboard->SetValueAsVector(PatrolTargetLocation, Value);
	}

	void ClearPatrolTargetLocation(UBlackboardComponent* Blackboard)
	{
		Blackboard->ClearValue(PatrolTargetLocation);
	}
}
