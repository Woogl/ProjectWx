// Copyright Woogle. All Rights Reserved.

#include "Actor/WxElevator.h"

#include "Component/WxInteractionComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/WxPromptWidgetComponent.h"
#include "Net/UnrealNetwork.h"

AWxElevator::AWxElevator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	
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

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(PlatformRoot);

	PromptWidget = CreateDefaultSubobject<UWxPromptWidgetComponent>(TEXT("PromptWidget"));
	PromptWidget->SetupAttachment(PlatformRoot);
	PromptWidget->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
}

void AWxElevator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxElevator, bIsMoving);
	DOREPLIFETIME(AWxElevator, bMovingForward);
	DOREPLIFETIME(AWxElevator, CurrentDistance);
}

void AWxElevator::BeginPlay()
{
	Super::BeginPlay();

	CachedSplineLength = SplineComponent->GetSplineLength();

	UpdatePlatformPosition();

	if (InteractionComponent)
	{
		InteractionComponent->OnInteracted.AddDynamic(this, &AWxElevator::HandleInteracted);
	}

	if (bIsMoving)
	{
		SetActorTickEnabled(true);
		InteractionComponent->SetInteractionEnabled(false);
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

	UpdatePlatformPosition();

	const bool bReachedEnd = bMovingForward
		? (CurrentDistance >= CachedSplineLength)
		: (CurrentDistance <= 0.f);

	if (bReachedEnd)
	{
		bIsMoving = false;
		bMovingForward = !bMovingForward;
		SetActorTickEnabled(false);
		InteractionComponent->SetInteractionEnabled(true);
	}
}

void AWxElevator::HandleInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || bIsMoving)
	{
		return;
	}

	bIsMoving = true;
	SetActorTickEnabled(true);
	InteractionComponent->SetInteractionEnabled(false);
}

void AWxElevator::OnRep_bIsMoving()
{
	SetActorTickEnabled(bIsMoving);
	InteractionComponent->SetInteractionEnabled(!bIsMoving);
}

void AWxElevator::UpdatePlatformPosition()
{
	const FVector NewLocation = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local);

	PlatformRoot->SetRelativeLocation(NewLocation);
}
