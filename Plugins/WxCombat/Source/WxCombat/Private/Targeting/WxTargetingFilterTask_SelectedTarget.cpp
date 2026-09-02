// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingFilterTask_SelectedTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "Targeting/WxLockOnComponent.h"
#include "Types/TargetingSystemTypes.h"

bool UWxTargetingFilterTask_SelectedTarget::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	const AActor* SourceActor = SourceContext ? SourceContext->SourceActor.Get() : nullptr;
	if (!SourceActor)
	{
		return false;
	}

	const AActor* SelectedTarget = nullptr;
	if (const UWxLockOnComponent* LockOnComponent = SourceActor->FindComponentByClass<UWxLockOnComponent>())
	{
		const USceneComponent* LockOnTarget = LockOnComponent->GetLockOnTarget();
		SelectedTarget = LockOnTarget ? LockOnTarget->GetOwner() : nullptr;
	}

	if (!SelectedTarget && !BlackboardTargetKey.IsNone())
	{
		const APawn* SourcePawn = Cast<APawn>(SourceActor);
		const AAIController* AIController = SourcePawn ? Cast<AAIController>(SourcePawn->GetController()) : nullptr;
		const UBlackboardComponent* Blackboard = AIController ? AIController->GetBlackboardComponent() : nullptr;
		SelectedTarget = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(BlackboardTargetKey)) : nullptr;
	}

	return SelectedTarget && TargetData.HitResult.GetActor() != SelectedTarget;
}
