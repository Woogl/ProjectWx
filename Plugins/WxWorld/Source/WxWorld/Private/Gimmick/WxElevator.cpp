// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxElevator.h"

#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/WxInteractionWidgetComponent.h"
#include "Net/UnrealNetwork.h"

AWxElevator::AWxElevator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CurrentDistance = 0.f;
	CachedSplineLength = 0.f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(SceneRoot);

	PlatformRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlatformRoot"));
	PlatformRoot->SetupAttachment(SceneRoot);

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	PlatformMesh->SetupAttachment(PlatformRoot);

	InteractionWidget->SetupAttachment(PlatformRoot);
	InteractionComponent->SetupAttachment(PlatformRoot);
}

void AWxElevator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxElevator, bIsMoving);
	DOREPLIFETIME(AWxElevator, bMovingForward);
	DOREPLIFETIME_CONDITION(AWxElevator, CurrentDistance, COND_InitialOnly);
}

void AWxElevator::BeginPlay()
{
	Super::BeginPlay();

	CachedSplineLength = SplineComponent->GetSplineLength();

	UpdatePlatformPosition();

	OnInteracted.AddDynamic(this, &AWxElevator::HandleInteracted);

	if (bIsMoving)
	{
		SetActorTickEnabled(true);
		SetInteractionEnabled(false);
	}
}

void AWxElevator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsMoving || CachedSplineLength <= 0.f)
	{
		return;
	}

	const float Direction = bMovingForward ? 1.f : -1.f;
	CurrentDistance = FMath::Clamp(CurrentDistance + MoveSpeed * DeltaTime * Direction, 0.f, CachedSplineLength);

	if (HasAuthority())
	{
		const bool bReachedEnd = bMovingForward
			? (CurrentDistance >= CachedSplineLength)
			: (CurrentDistance <= 0.f);

		if (bReachedEnd)
		{
			bIsMoving = false;
			bMovingForward = !bMovingForward;
			SetActorTickEnabled(false);
			SetInteractionEnabled(true);
		}
	}

	UpdatePlatformPosition();
}

void AWxElevator::HandleInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || bIsMoving)
	{
		return;
	}

	bIsMoving = true;
	SetActorTickEnabled(true);
	SetInteractionEnabled(false);
}

void AWxElevator::OnRep_bIsMoving()
{
	SetActorTickEnabled(bIsMoving);
	SetInteractionEnabled(!bIsMoving);

	// 정지 복제 수신 시 누적 드리프트를 끝점으로 스냅 (BeginPlay 이후에만).
	// bMovingForward는 서버에서 정지와 동시에 이미 반전됐으므로 다음 진행 방향 기준으로 역의 끝점이 현재 위치.
	if (!bIsMoving && CachedSplineLength > 0.f)
	{
		CurrentDistance = bMovingForward ? 0.f : CachedSplineLength;
		UpdatePlatformPosition();
	}
}

void AWxElevator::UpdatePlatformPosition()
{
	const FVector NewLocation = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local);

	PlatformRoot->SetRelativeLocation(NewLocation);
}
