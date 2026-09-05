// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingSorterTask_InputDirection.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Types/TargetingSystemTypes.h"

float UWxTargetingSorterTask_InputDirection::GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext)
	{
		return 0.f;
	}

	const APawn* SourcePawn = Cast<APawn>(SourceContext->SourceActor);
	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!SourcePawn || !TargetActor)
	{
		return 0.f;
	}

	// GetLastMovementInputVector는 AddMovementInput을 부른 머신에서만 채워져 서버에서는 원격 폰이 항상 0이 된다.
	const UCharacterMovementComponent* MovementComponent = Cast<UCharacterMovementComponent>(SourcePawn->GetMovementComponent());
	if (!MovementComponent)
	{
		return 0.f;
	}

	// 이동 입력은 평면적이라 yaw 기준으로만 비교한다.
	FVector InputDir = MovementComponent->GetCurrentAcceleration();
	InputDir.Z = 0.f;
	InputDir = InputDir.GetSafeNormal();

	FVector ToTarget = TargetActor->GetActorLocation() - SourcePawn->GetActorLocation();
	ToTarget.Z = 0.f;
	ToTarget = ToTarget.GetSafeNormal();

	// 수평으로 겹쳐 방향을 못 구하는 타겟은 사이각 0, 즉 최우선으로 취급된다.
	if (InputDir.IsNearlyZero() || ToTarget.IsNearlyZero())
	{
		return 0.f;
	}

	const float Dot = FMath::Clamp(FVector::DotProduct(InputDir, ToTarget), -1.f, 1.f);
	return FMath::RadiansToDegrees(FMath::Acos(Dot));
}
