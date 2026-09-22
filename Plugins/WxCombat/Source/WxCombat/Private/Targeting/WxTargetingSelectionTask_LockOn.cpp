// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingSelectionTask_LockOn.h"

#include "GameFramework/Actor.h"
#include "Targeting/WxLockOnComponent.h"
#include "Types/TargetingSystemTypes.h"

void UWxTargetingSelectionTask_LockOn::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
	Super::Execute(TargetingHandle);
	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

	if (TargetingHandle.IsValid())
	{
		const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
		const AActor* SourceActor = SourceContext ? SourceContext->SourceActor.Get() : nullptr;
		AActor* LockOnTarget = IsValid(SourceActor) ? UWxLockOnComponent::ResolveLockOnTargetActor(SourceActor) : nullptr;
		if (IsValid(LockOnTarget))
		{
			TArray<FTargetingDefaultResultData>& Results = FTargetingDefaultResultsSet::FindOrAdd(TargetingHandle).TargetResults;
			const int32 ExistingIndex = Results.IndexOfByPredicate([LockOnTarget](const FTargetingDefaultResultData& Result)
			{
				return Result.HitResult.GetActor() == LockOnTarget;
			});

			if (ExistingIndex != 0)
			{
				FTargetingDefaultResultData LockOnResult;
				if (ExistingIndex != INDEX_NONE)
				{
					// AOE의 충돌 정보와 나머지 후보의 상대 순서를 보존한다.
					LockOnResult = MoveTemp(Results[ExistingIndex]);
					Results.RemoveAt(ExistingIndex);
				}
				else
				{
					LockOnResult.HitResult.HitObjectHandle = FActorInstanceHandle(LockOnTarget);
					LockOnResult.HitResult.Location = LockOnTarget->GetActorLocation();
					LockOnResult.HitResult.ImpactPoint = LockOnResult.HitResult.Location;
					LockOnResult.HitResult.TraceStart = SourceActor->GetActorLocation();
					LockOnResult.HitResult.Distance = FVector::Distance(LockOnResult.HitResult.TraceStart, LockOnResult.HitResult.Location);
				}
				Results.Insert(MoveTemp(LockOnResult), 0);
			}
		}
	}

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}
