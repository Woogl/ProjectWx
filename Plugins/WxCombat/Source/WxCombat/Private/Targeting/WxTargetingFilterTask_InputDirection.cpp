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

	if (PassesInputCone(*SourcePawn, InputDir, *TargetActor))
	{
		return false;
	}

	// 이 옵션이 꺼져 있으면 콘 밖 타겟은 항상 제외한다.
	if (!bKeepAllWhenNoMatch)
	{
		return true;
	}

	// 콘 안에 드는 후보가 하나라도 있으면 이 콘 밖 타겟을 제외하고, 하나도 없으면(전부 콘 밖) 전체를 유지한다.
	const FTargetingDefaultResultsSet* ResultData = FTargetingDefaultResultsSet::Find(TargetingHandle);
	if (!ResultData)
	{
		return false;
	}

	for (const FTargetingDefaultResultData& OtherResult : ResultData->TargetResults)
	{
		const AActor* OtherActor = OtherResult.HitResult.GetActor();
		if (OtherActor && PassesInputCone(*SourcePawn, InputDir, *OtherActor))
		{
			return true;
		}
	}

	return false;
}

bool UWxTargetingFilterTask_InputDirection::PassesInputCone(const APawn& SourcePawn, const FVector& InputDirNormalized, const AActor& TargetActor) const
{
	FVector ToTarget = TargetActor.GetActorLocation() - SourcePawn.GetActorLocation();
	ToTarget.Z = 0.f;
	ToTarget = ToTarget.GetSafeNormal();
	if (ToTarget.IsNearlyZero())
	{
		// 방향을 못 구하면(수직으로 겹침 등) 콘 밖으로 보지 않고 통과로 취급한다.
		return true;
	}

	// 입력 방향과 타겟 방향의 각이 MaxInputAngle 이내면(= Dot 가 임계 이상) 콘 안이다.
	const float Dot = FVector::DotProduct(InputDirNormalized, ToTarget);
	return Dot >= FMath::Cos(FMath::DegreesToRadians(MaxInputAngle));
}
