// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingFilterTask_CameraDirection.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Types/TargetingSystemTypes.h"

bool UWxTargetingFilterTask_CameraDirection::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext)
	{
		return false;
	}

	const APawn* SourcePawn = Cast<APawn>(SourceContext->SourceActor);
	const AController* Controller = SourcePawn ? SourcePawn->GetController() : nullptr;
	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!Controller || !TargetActor)
	{
		return false;
	}

	// 플레이어면 카메라 매니저 기준 시점, AI 면 폰/컨트롤 회전 기준 시점.
	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector ToTarget = (TargetActor->GetActorLocation() - ViewLocation).GetSafeNormal();
	if (ToTarget.IsNearlyZero())
	{
		return false;
	}

	// 정면과 타겟 방향의 각이 MaxViewAngle 을 넘으면(= Dot 가 임계 미만) 제외한다.
	const float Dot = FVector::DotProduct(ViewRotation.Vector(), ToTarget);
	return Dot < FMath::Cos(FMath::DegreesToRadians(MaxViewAngle));
}
