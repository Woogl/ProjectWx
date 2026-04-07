// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingFilterTask_LineTrace.h"
#include "Types/TargetingSystemTypes.h"

bool UWxTargetingFilterTask_LineTrace::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext)
	{
		return false;
	}

	const AActor* SourceActor = SourceContext->SourceActor;
	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!SourceActor || !TargetActor)
	{
		return false;
	}

	const UWorld* World = SourceActor->GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector TraceStart = SourceActor->GetActorLocation() + SourceActor->GetActorRotation().RotateVector(SourceOffset);
	const FVector TraceEnd = TargetActor->GetActorLocation() + TargetActor->GetActorRotation().RotateVector(TargetOffset);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(SourceActor);
	QueryParams.AddIgnoredActor(TargetActor);

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, TraceChannel, QueryParams);

	// 트레이스가 차단되면 타겟을 필터링(제외)한다.
	return bHit;
}
