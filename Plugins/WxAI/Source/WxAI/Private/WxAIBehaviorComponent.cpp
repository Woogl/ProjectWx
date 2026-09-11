// Copyright Woogle. All Rights Reserved.

#include "WxAIBehaviorComponent.h"
#include "WxAIModule.h"
#include "GameFramework/Pawn.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#endif

UWxAIBehaviorComponent::UWxAIBehaviorComponent()
{
#if WITH_EDITOR
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
#endif
}

void UWxAIBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	// PIE에서도 저작용 드로우를 위한 Tick은 필요하지 않다.
	SetComponentTickEnabled(false);
#endif

	if (!GetOwner<APawn>())
	{
		UE_LOG(LogWxAI, Warning, TEXT("%s: WxAIBehaviorComponent 는 Pawn 에만 부착할 수 있다."), *GetNameSafe(GetOwner()));
	}
}

#if WITH_EDITOR
void UWxAIBehaviorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	const APawn* Pawn = GetOwner<APawn>();
	if (!World || World->WorldType != EWorldType::Editor || !Pawn)
	{
		return;
	}

	// ChildActor 미리보기는 Pawn 대신 이를 배치한 부모 액터가 선택된다.
	const AActor* SelectedActor = Pawn;
	while (SelectedActor && !SelectedActor->IsSelected())
	{
		SelectedActor = SelectedActor->GetParentActor();
	}
	if (!SelectedActor)
	{
		return;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	Pawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const float Radius = FMath::Max(0.f, SightRadius);
	const float HalfAngle = FMath::Clamp(SightAngle, 0.f, 180.f);
	const FColor Color = FColor::Green;
	const FVector Forward = EyeRotation.Vector();
	const FVector Right = FRotationMatrix(EyeRotation).GetUnitAxis(EAxis::Y);
	const float StartRadians = FMath::DegreesToRadians(-HalfAngle);
	FVector PreviousPoint = EyeLocation + Radius * (Forward * FMath::Cos(StartRadians) + Right * FMath::Sin(StartRadians));
	DrawDebugLine(World, EyeLocation, PreviousPoint, Color);

	// 거리 제한과 편측 각도를 수평 단면으로 표시한다. 180도는 원 전체가 된다.
	constexpr int32 SegmentCount = 64;
	for (int32 Segment = 1; Segment <= SegmentCount; ++Segment)
	{
		const float AngleRadians = FMath::DegreesToRadians(FMath::Lerp(-HalfAngle, HalfAngle, static_cast<float>(Segment) / SegmentCount));
		const FVector Point = EyeLocation + Radius * (Forward * FMath::Cos(AngleRadians) + Right * FMath::Sin(AngleRadians));
		DrawDebugLine(World, PreviousPoint, Point, Color);
		PreviousPoint = Point;
	}
	DrawDebugLine(World, EyeLocation, PreviousPoint, Color);
}
#endif

UBehaviorTree* UWxAIBehaviorComponent::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

float UWxAIBehaviorComponent::GetSightRadius() const
{
	return SightRadius;
}

float UWxAIBehaviorComponent::GetSightAngle() const
{
	return SightAngle;
}

float UWxAIBehaviorComponent::GetHearingRadius() const
{
	return HearingRadius;
}
