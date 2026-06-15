// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingFilterTask_InputDirection.h"
#include "GameFramework/Pawn.h"
#include "Types/TargetingSystemTypes.h"

bool UWxTargetingFilterTask_InputDirection::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext)
	{
		return false;
	}

	const APawn* SourcePawn = Cast<APawn>(SourceContext->SourceActor);
	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!SourcePawn || !TargetActor)
	{
		return false;
	}

	// 이동 입력은 평면적이라 수평(XY)으로 평탄화해 yaw 기준으로 비교한다.
	FVector InputDir = SourcePawn->GetLastMovementInputVector();
	InputDir.Z = 0.f;

	// 입력이 없으면(스틱 중립) 아무것도 제외하지 않는다.
	if (InputDir.IsNearlyZero())
	{
		return false;
	}
	InputDir = InputDir.GetSafeNormal();

	FVector ToTarget = TargetActor->GetActorLocation() - SourcePawn->GetActorLocation();
	ToTarget.Z = 0.f;
	ToTarget = ToTarget.GetSafeNormal();
	if (ToTarget.IsNearlyZero())
	{
		return false;
	}

	// 입력 방향과 타겟 방향의 각이 MaxInputAngle 을 넘으면(= Dot 가 임계 미만) 제외한다.
	const float Dot = FVector::DotProduct(InputDir, ToTarget);
	return Dot < FMath::Cos(FMath::DegreesToRadians(MaxInputAngle));
}
