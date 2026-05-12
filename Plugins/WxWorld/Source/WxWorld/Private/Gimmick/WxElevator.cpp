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

	CurrentDistance = 0.f;
	CachedSplineLength = 0.f;

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
	DOREPLIFETIME(AWxElevator, TargetDistance);
	DOREPLIFETIME_CONDITION(AWxElevator, CurrentDistance, COND_InitialOnly);
}

void AWxElevator::BeginPlay()
{
	Super::BeginPlay();

	CachedSplineLength = SplineComponent->GetSplineLength();

	// 문 닫힘 위치 캐시: BP/레벨에서 배치된 상대 위치를 닫힘 기준으로 사용.
	// 레이트조인 시 OnRep_State 가 BeginPlay 이후 발화하므로 그 시점에 캐시가 유효해야 한다.
	DoorLeftClosedLocation = DoorLeft->GetRelativeLocation();
	DoorRightClosedLocation = DoorRight->GetRelativeLocation();

	// CurrentDistance 는 영구화 대상이 아니므로 영구화된 TargetDistance 로 강제 동기화.
	// 정지 상태에선 둘이 동일하고, Moving 도중 언로드된 케이스는 리로드 후 Tick 에서 즉시 도착 처리된다.
	CurrentDistance = TargetDistance;
	UpdatePlatformPosition();

	PlatformInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandlePlatformInteracted);
	CallConsoleAInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandleCallConsoleAInteracted);
	CallConsoleBInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandleCallConsoleBInteracted);

	// Level Streaming Persistence 로 State/TargetDistance 가 BeginPlay 직전에 직접 set 되므로
	// OnRep 이 발화하지 않는다. 영구화된 State 를 시각/인터랙션에 반영하기 위해 ApplyState 를 명시 호출.
	ApplyState();
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
		const float Direction = TargetDistance > CurrentDistance ? 1.f : -1.f;
		const float Speed = MoveDuration > 0.f ? CachedSplineLength / MoveDuration : CachedSplineLength;
		CurrentDistance = FMath::Clamp(CurrentDistance + Speed * DeltaTime * Direction, 0.f, CachedSplineLength);
		UpdatePlatformPosition();

		if (HasAuthority())
		{
			const bool bReachedEnd = (Direction > 0.f) ? (CurrentDistance >= TargetDistance) : (CurrentDistance <= TargetDistance);

			if (bReachedEnd)
			{
				State = EWxElevatorState::DoorsOpening;
				ApplyState();
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
			ApplyState();
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
			ApplyState();
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

	// DoorsOpen 상태에서는 항상 끝점에 정지해 있다. 시작점(0)이면 끝점으로, 끝점이면 시작점으로 호출.
	const float NewTarget = FMath::IsNearlyZero(CurrentDistance) ? CachedSplineLength : 0.f;
	BeginMoveSequence(NewTarget);
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

void AWxElevator::BeginMoveSequence(float NewTargetDistance)
{
	// 이미 목표 끝점에 정지해 있는 케이스: 문이 닫혀 있다면 열기만 수행, 열려 있다면 호출은 의미 없으므로 무시.
	if (FMath::IsNearlyEqual(CurrentDistance, NewTargetDistance))
	{
		if (State == EWxElevatorState::DoorsClosed)
		{
			State = EWxElevatorState::DoorsOpening;
			ApplyState();
		}
		return;
	}

	TargetDistance = NewTargetDistance;

	if (State == EWxElevatorState::DoorsOpen)
	{
		State = EWxElevatorState::DoorsClosing;
		ApplyState();
	}
	else
	{
		// 문이 이미 닫혀 있으므로 DoorsClosing 단계 건너뛰고 즉시 이동 시작.
		State = EWxElevatorState::Moving;
		ApplyState();
	}
}

void AWxElevator::OnRep_State()
{
	ApplyState();
}

void AWxElevator::ApplyState()
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
		// 클라이언트의 누적 드리프트를 목표 끝점으로 스냅.
		CurrentDistance = TargetDistance;
		UpdatePlatformPosition();
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
