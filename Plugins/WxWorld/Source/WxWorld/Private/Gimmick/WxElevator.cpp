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

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(SceneRoot);

	PlatformRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlatformRoot"));
	PlatformRoot->SetupAttachment(SceneRoot);

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	PlatformMesh->SetupAttachment(PlatformRoot);

	DoorLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorLeft"));
	DoorLeft->SetupAttachment(PlatformRoot);

	DoorRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorRight"));
	DoorRight->SetupAttachment(PlatformRoot);

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

	DOREPLIFETIME(AWxElevator, State);
	DOREPLIFETIME(AWxElevator, bMovingForward);
	DOREPLIFETIME_CONDITION(AWxElevator, CurrentDistance, COND_InitialOnly);
}

void AWxElevator::BeginPlay()
{
	Super::BeginPlay();

	CachedSplineLength = SplineComponent->GetSplineLength();

	// 문 닫힘 위치 캐시: BP/레벨에서 배치된 상대 위치를 닫힘 기준으로 사용.
	DoorLeftClosedLocation = DoorLeft->GetRelativeLocation();
	DoorRightClosedLocation = DoorRight->GetRelativeLocation();

	UpdatePlatformPosition();

	PlatformInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandlePlatformInteracted);
	CallConsoleAInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandleCallConsoleAInteracted);
	CallConsoleBInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandleCallConsoleBInteracted);

	// 레이트조인: 진행 중인 시퀀스 상태에 맞춰 시각 효과/틱/인터랙션 정리. (DoorsClosed/DoorsOpen 이면 no-op 에 가까움)
	ApplyStateSideEffects();
}

void AWxElevator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CachedSplineLength <= 0.f)
	{
		return;
	}

	switch (State)
	{
	case EWxElevatorState::Moving:
	{
		const float Direction = bMovingForward ? 1.f : -1.f;
		CurrentDistance = FMath::Clamp(CurrentDistance + MoveSpeed * DeltaTime * Direction, 0.f, CachedSplineLength);
		UpdatePlatformPosition();

		if (HasAuthority())
		{
			const bool bReachedEnd = bMovingForward ? (CurrentDistance >= CachedSplineLength) : (CurrentDistance <= 0.f);

			if (bReachedEnd)
			{
				// bMovingForward 는 다음 이동 방향(=직전에 도착한 끝점의 반대)을 의미하도록 토글. OnRep 측 끝점 스냅 판정에 사용됨.
				bMovingForward = !bMovingForward;
				State = EWxElevatorState::DoorsOpening;
				ApplyStateSideEffects();
			}
		}
		break;
	}

	case EWxElevatorState::DoorsOpening:
	{
		const float Step = DoorAnimDuration > 0.f ? DeltaTime / DoorAnimDuration : 1.f;
		DoorAnimProgress = FMath::Clamp(DoorAnimProgress + Step, 0.f, 1.f);
		UpdateDoorPositions();

		if (HasAuthority() && DoorAnimProgress >= 1.f)
		{
			State = EWxElevatorState::DoorsOpen;
			ApplyStateSideEffects();
		}
		break;
	}

	case EWxElevatorState::DoorsClosing:
	{
		const float Step = DoorAnimDuration > 0.f ? DeltaTime / DoorAnimDuration : 1.f;
		DoorAnimProgress = FMath::Clamp(DoorAnimProgress - Step, 0.f, 1.f);
		UpdateDoorPositions();

		if (HasAuthority() && DoorAnimProgress <= 0.f)
		{
			State = EWxElevatorState::Moving;
			ApplyStateSideEffects();
		}
		break;
	}

	default:
		break;
	}
}

void AWxElevator::HandlePlatformInteracted(AActor* InteractingActor)
{
	// 플랫폼 위 인터랙션은 문이 열려 있을 때만 의미가 있다 (닫혀 있다면 플레이어가 탑승해 있을 수 없음).
	if (!HasAuthority() || State != EWxElevatorState::DoorsOpen)
	{
		return;
	}

	// DoorsOpen 상태에서는 항상 끝점에 정지해 있으며, bMovingForward 는 그 끝점에서의 다음 진행 방향을 의미한다.
	// 시작점(0)에 있으면 bMovingForward=true → 끝점으로, 끝점(SplineLength)에 있으면 false → 시작점으로.
	const float TargetDistance = bMovingForward ? CachedSplineLength : 0.f;
	BeginMoveSequence(TargetDistance);
}

void AWxElevator::HandleCallConsoleAInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || (State != EWxElevatorState::DoorsClosed && State != EWxElevatorState::DoorsOpen))
	{
		return;
	}

	BeginMoveSequence(0.f);
}

void AWxElevator::HandleCallConsoleBInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || (State != EWxElevatorState::DoorsClosed && State != EWxElevatorState::DoorsOpen))
	{
		return;
	}

	BeginMoveSequence(CachedSplineLength);
}

void AWxElevator::MovePlatformToStart()
{
	if (!HasAuthority() || (State != EWxElevatorState::DoorsClosed && State != EWxElevatorState::DoorsOpen))
	{
		return;
	}

	BeginMoveSequence(0.f);
}

void AWxElevator::MovePlatformToEnd()
{
	if (!HasAuthority() || (State != EWxElevatorState::DoorsClosed && State != EWxElevatorState::DoorsOpen))
	{
		return;
	}

	BeginMoveSequence(CachedSplineLength);
}

void AWxElevator::BeginMoveSequence(float TargetDistance)
{
	// 이미 목표 끝점에 정지해 있는 케이스: 문이 닫혀 있다면 열기만 수행, 열려 있다면 호출은 의미 없으므로 무시.
	if (FMath::IsNearlyEqual(CurrentDistance, TargetDistance))
	{
		if (State == EWxElevatorState::DoorsClosed)
		{
			State = EWxElevatorState::DoorsOpening;
			ApplyStateSideEffects();
		}
		return;
	}

	bMovingForward = TargetDistance > CurrentDistance;

	if (State == EWxElevatorState::DoorsOpen)
	{
		State = EWxElevatorState::DoorsClosing;
		ApplyStateSideEffects();
	}
	else
	{
		// 문이 이미 닫혀 있으므로 DoorsClosing 단계 건너뛰고 즉시 이동 시작.
		State = EWxElevatorState::Moving;
		ApplyStateSideEffects();
	}
}

void AWxElevator::OnRep_State()
{
	ApplyStateSideEffects();
}

void AWxElevator::ApplyStateSideEffects()
{
	switch (State)
	{
	case EWxElevatorState::DoorsClosed:
		SetActorTickEnabled(false);
		SetAllInteractionsEnabled(true);
		DoorAnimProgress = 0.f;
		UpdateDoorPositions();
		break;

	case EWxElevatorState::DoorsOpening:
		SetActorTickEnabled(true);
		SetAllInteractionsEnabled(false);
		// 클라이언트의 누적 드리프트를 끝점으로 스냅. bMovingForward 는 도착 시점에 다음 진행 방향으로 토글되어 있다.
		if (CachedSplineLength > 0.f)
		{
			CurrentDistance = bMovingForward ? 0.f : CachedSplineLength;
			UpdatePlatformPosition();
		}
		// DoorAnimProgress 는 진입 직전 값(Moving/DoorsClosed 에서 0)을 그대로 사용; Tick 이 1까지 증가시킴.
		break;

	case EWxElevatorState::DoorsOpen:
		SetActorTickEnabled(false);
		SetAllInteractionsEnabled(true);
		DoorAnimProgress = 1.f;
		UpdateDoorPositions();
		break;

	case EWxElevatorState::DoorsClosing:
		SetActorTickEnabled(true);
		SetAllInteractionsEnabled(false);
		// DoorAnimProgress 는 진입 직전 값(DoorsOpen 에서 1)을 그대로 사용; Tick 이 0까지 감소시킴.
		break;

	case EWxElevatorState::Moving:
		SetActorTickEnabled(true);
		SetAllInteractionsEnabled(false);
		DoorAnimProgress = 0.f;
		UpdateDoorPositions();
		break;
	}
}

void AWxElevator::UpdatePlatformPosition()
{
	const FVector NewLocation = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local);

	PlatformRoot->SetRelativeLocation(NewLocation);
}

void AWxElevator::UpdateDoorPositions()
{
	const FVector Offset = DoorOpenOffset * DoorAnimProgress;

	DoorLeft->SetRelativeLocation(DoorLeftClosedLocation - Offset);
	DoorRight->SetRelativeLocation(DoorRightClosedLocation + Offset);
}

void AWxElevator::SetAllInteractionsEnabled(bool bEnabled)
{
	PlatformInteraction->SetInteractionEnabled(bEnabled);
	CallConsoleAInteraction->SetInteractionEnabled(bEnabled);
	CallConsoleBInteraction->SetInteractionEnabled(bEnabled);
}
