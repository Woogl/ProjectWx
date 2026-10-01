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

			// 범위 밖 락온 대상은 넣지 않는다 — 스냅 이동과 범위 대미지가 결과 포함 여부를 범위 판정으로 쓴다.
			if (ExistingIndex > 0)
			{
				// AOE의 충돌 정보와 나머지 후보의 상대 순서를 보존한다.
				FTargetingDefaultResultData LockOnResult = MoveTemp(Results[ExistingIndex]);
				Results.RemoveAt(ExistingIndex);
				Results.Insert(MoveTemp(LockOnResult), 0);
			}
		}
	}

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}
