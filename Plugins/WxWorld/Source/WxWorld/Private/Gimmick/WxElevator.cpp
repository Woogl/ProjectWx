// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxElevator.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"

AWxElevator::AWxElevator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;

	CurrentDistance = 0.f;
	CachedSplineLength = 0.f;
	CallConsoleATargetDistance = 0.f;
	CallConsoleBTargetDistance = 0.f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(SceneRoot);

	PlatformRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlatformRoot"));
	PlatformRoot->SetupAttachment(SceneRoot);

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	PlatformMesh->SetupAttachment(PlatformRoot);

	PlatformInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("PlatformInteraction"));
	PlatformInteraction->SetupAttachment(PlatformRoot);

	CallConsoleA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CallConsoleA"));
	CallConsoleA->SetupAttachment(SceneRoot);

	CallConsoleAInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("CallConsoleAInteraction"));
	CallConsoleAInteraction->SetupAttachment(CallConsoleA);

	CallConsoleB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CallConsoleB"));
	CallConsoleB->SetupAttachment(SceneRoot);

	CallConsoleBInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("CallConsoleBInteraction"));
	CallConsoleBInteraction->SetupAttachment(CallConsoleB);
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

	// 콘솔 끝점 매핑: 콘솔 월드 위치를 스플라인 양 끝점과 비교해 가까운 쪽 거리값을 목적지로 캐싱.
	const FVector StartWorld = SplineComponent->GetLocationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World);
	const FVector EndWorld = SplineComponent->GetLocationAtDistanceAlongSpline(CachedSplineLength, ESplineCoordinateSpace::World);

	const FVector ConsoleAWorld = CallConsoleA->GetComponentLocation();
	CallConsoleATargetDistance = (FVector::DistSquared(ConsoleAWorld, StartWorld) <= FVector::DistSquared(ConsoleAWorld, EndWorld))
		? 0.f
		: CachedSplineLength;

	const FVector ConsoleBWorld = CallConsoleB->GetComponentLocation();
	CallConsoleBTargetDistance = (FVector::DistSquared(ConsoleBWorld, StartWorld) <= FVector::DistSquared(ConsoleBWorld, EndWorld))
		? 0.f
		: CachedSplineLength;

	PlatformInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandlePlatformInteracted);
	CallConsoleAInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandleCallConsoleAInteracted);
	CallConsoleBInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandleCallConsoleBInteracted);

	if (bIsMoving)
	{
		SetActorTickEnabled(true);
		SetAllInteractionsEnabled(false);
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
			SetAllInteractionsEnabled(true);
		}
	}

	UpdatePlatformPosition();
}

void AWxElevator::HandlePlatformInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || bIsMoving)
	{
		return;
	}

	bIsMoving = true;
	SetActorTickEnabled(true);
	SetAllInteractionsEnabled(false);
}

void AWxElevator::HandleCallConsoleAInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || bIsMoving)
	{
		return;
	}

	StartMovementToDistance(CallConsoleATargetDistance);
}

void AWxElevator::HandleCallConsoleBInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || bIsMoving)
	{
		return;
	}

	StartMovementToDistance(CallConsoleBTargetDistance);
}

void AWxElevator::StartMovementToDistance(float TargetDistance)
{
	if (FMath::IsNearlyEqual(CurrentDistance, TargetDistance))
	{
		return;
	}

	bMovingForward = TargetDistance > CurrentDistance;
	bIsMoving = true;
	SetActorTickEnabled(true);
	SetAllInteractionsEnabled(false);
}

void AWxElevator::OnRep_bIsMoving()
{
	SetActorTickEnabled(bIsMoving);
	SetAllInteractionsEnabled(!bIsMoving);

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

void AWxElevator::SetAllInteractionsEnabled(bool bEnabled)
{
	PlatformInteraction->SetInteractionEnabled(bEnabled);
	CallConsoleAInteraction->SetInteractionEnabled(bEnabled);
	CallConsoleBInteraction->SetInteractionEnabled(bEnabled);
}
