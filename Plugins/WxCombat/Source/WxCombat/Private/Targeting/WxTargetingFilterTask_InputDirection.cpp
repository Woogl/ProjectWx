// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingFilterTask_InputDirection.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	// GetLastMovementInputVector는 AddMovementInput을 부른 머신에서만 채워져 서버에서는 원격 폰이 항상 0이 된다.
	// CMC의 Acceleration은 서버가 ServerMove로 받은 클라이언트 값을 그대로 넣으므로, 같은 프리셋을 도는 모든 머신이 같은 판정을 낸다.
	const UCharacterMovementComponent* MovementComponent = Cast<UCharacterMovementComponent>(SourcePawn->GetMovementComponent());
	if (!MovementComponent)
	{
		return false;
	}

	// 이동 입력은 평면적이라 수평(XY)으로 평탄화해 yaw 기준으로 비교한다.
	FVector InputDir = MovementComponent->GetCurrentAcceleration();
	InputDir.Z = 0.f;

	if (InputDir.IsNearlyZero())
	{
		return false;
	}
	InputDir = InputDir.GetSafeNormal();

	if (PassesInputCone(*SourcePawn, InputDir, *TargetActor))
	{
		return false;
	}

	if (!bKeepAllWhenNoMatch)
	{
		return true;
	}

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

	const float Dot = FVector::DotProduct(InputDirNormalized, ToTarget);
	return Dot >= FMath::Cos(FMath::DegreesToRadians(MaxInputAngle));
}
