// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxElevator.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
	DOREPLIFETIME(AWxElevator, TargetEndpoint);
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

	// 각 문은 자기 메시 너비만큼 바깥쪽(좌: -Y, 우: +Y)으로 슬라이드.
	DoorLeftOpenOffset = FVector(0.f, -ComputeDoorWidth(DoorLeft), 0.f);
	DoorRightOpenOffset = FVector(0.f, ComputeDoorWidth(DoorRight), 0.f);

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

void AWxElevator::ApplyState()
{
	// TargetEndpoint = 이 시퀀스의 최종 목적지. 정지 상태(Closed/Open) 와 도착 직후(DoorsOpening) 에선 현재 위치 = TargetEndpoint.
	// 반면 DoorsClosing (출발 직전 정지) 과 Moving (이동 중) 에선 출발 끝점 = TargetEndpoint 의 반대.
	const float TargetEndpointDistance = TargetEndpoint == EWxElevatorEndpoint::End ? CachedSplineLength : 0.f;
	const float OppositeEndpointDistance = TargetEndpoint == EWxElevatorEndpoint::End ? 0.f : CachedSplineLength;

	switch (State)
	{
	case EWxElevatorState::DoorsClosed:
		SetActorTickEnabled(false);
		SetAllInteractionsEnabled(true);
		CurrentDistance = TargetEndpointDistance;
		TargetDistance = TargetEndpointDistance;
		DoorAnimProgress = 0.f;
		UpdateDoorPositions();
		break;

	case EWxElevatorState::DoorsOpening:
		SetActorTickEnabled(true);
		SetAllInteractionsEnabled(false);
		// 도착 직후이거나 슬롯 복원 — 둘 다 목적지 끝점으로 스냅.
		CurrentDistance = TargetEndpointDistance;
		TargetDistance = TargetEndpointDistance;
		// DoorAnimProgress 는 진입 직전 값(Moving/DoorsClosed 에서 0)을 그대로 사용; Tick 이 1까지 증가시킴.
		break;

	case EWxElevatorState::DoorsOpen:
		SetActorTickEnabled(false);
		SetAllInteractionsEnabled(true);
		CurrentDistance = TargetEndpointDistance;
		TargetDistance = TargetEndpointDistance;
		DoorAnimProgress = 1.f;
		UpdateDoorPositions();
		break;

	case EWxElevatorState::DoorsClosing:
		SetActorTickEnabled(true);
		SetAllInteractionsEnabled(false);
		// 출발 끝점(목적지의 반대) 에 정지한 채 문 닫는 중.
		CurrentDistance = OppositeEndpointDistance;
		TargetDistance = TargetEndpointDistance;
		// DoorsClosing 은 항상 DoorsOpen(progress=1) 또는 슬롯 복원(default=0) 에서 진입.
		// 두 경우 모두 닫힘 애니가 1→0 으로 진행되어야 하므로 진입 시점에 1 로 스냅. Tick 이 0까지 감소시킴.
		DoorAnimProgress = 1.f;
		UpdateDoorPositions();
		break;

	case EWxElevatorState::Moving:
		SetActorTickEnabled(true);
		SetAllInteractionsEnabled(false);
		TargetDistance = TargetEndpointDistance;
		// 슬롯 복원 보정: CurrentDistance 가 디폴트(0)라 출발/목적이 같으면 반대 끝점에서 출발하도록 강제.
		// 정상 흐름의 BeginMoveSequence 는 CurrentDistance 가 현재 위치를 유지하므로 이 분기에 들어가지 않는다.
		if (FMath::IsNearlyEqual(CurrentDistance, TargetDistance))
		{
			CurrentDistance = OppositeEndpointDistance;
		}
		DoorAnimProgress = 0.f;
		UpdateDoorPositions();
		break;
	}

	// 모든 상태 적용 후 플랫폼을 CurrentDistance 에 동기화. 슬롯 복원 / OnRep / 상태 전이 모두 한 경로로 처리.
	UpdatePlatformPosition();
}

void AWxElevator::HandlePlatformInteracted(AActor* InteractingActor)
{
	// 플랫폼 위 인터랙션은 문이 열려 있을 때만 의미가 있다 (닫혀 있다면 플레이어가 탑승해 있을 수 없음).
	if (!HasAuthority() || State != EWxElevatorState::DoorsOpen)
	{
		return;
	}

	// 현재 endpoint 반대로 토글.
	BeginMoveSequence(TargetEndpoint == EWxElevatorEndpoint::Start ? EWxElevatorEndpoint::End : EWxElevatorEndpoint::Start);
}

void AWxElevator::HandleCallConsoleAInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || (State != EWxElevatorState::DoorsClosed && State != EWxElevatorState::DoorsOpen))
	{
		return;
	}

	BeginMoveSequence(EWxElevatorEndpoint::Start);
}

void AWxElevator::HandleCallConsoleBInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || (State != EWxElevatorState::DoorsClosed && State != EWxElevatorState::DoorsOpen))
	{
		return;
	}

	BeginMoveSequence(EWxElevatorEndpoint::End);
}

void AWxElevator::OnRep_State()
{
	ApplyState();
}

void AWxElevator::OnRep_TargetEndpoint()
{
	ApplyState();
}

void AWxElevator::MovePlatformToStart()
{
	if (!HasAuthority() || (State != EWxElevatorState::DoorsClosed && State != EWxElevatorState::DoorsOpen))
	{
		return;
	}

	BeginMoveSequence(EWxElevatorEndpoint::Start);
}

void AWxElevator::MovePlatformToEnd()
{
	if (!HasAuthority() || (State != EWxElevatorState::DoorsClosed && State != EWxElevatorState::DoorsOpen))
	{
		return;
	}

	BeginMoveSequence(EWxElevatorEndpoint::End);
}

void AWxElevator::BeginMoveSequence(EWxElevatorEndpoint NewEndpoint)
{
	const float NewTargetDistance = NewEndpoint == EWxElevatorEndpoint::End ? CachedSplineLength : 0.f;

	// 이미 목표 끝점에 정지해 있는 케이스: 문이 닫혀 있다면 열기만 수행, 열려 있다면 호출은 의미 없으므로 무시.
	if (NewEndpoint == TargetEndpoint && FMath::IsNearlyEqual(CurrentDistance, NewTargetDistance))
	{
		if (State == EWxElevatorState::DoorsClosed)
		{
			State = EWxElevatorState::DoorsOpening;
			ApplyState();
		}
		return;
	}

	TargetEndpoint = NewEndpoint;
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

void AWxElevator::UpdatePlatformPosition()
{
	const FVector NewLocation = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local);

	PlatformRoot->SetRelativeLocation(NewLocation);
}

void AWxElevator::UpdateDoorPositions()
{
	DoorLeft->SetRelativeLocation(DoorLeftClosedLocation + DoorLeftOpenOffset * DoorAnimProgress);
	DoorRight->SetRelativeLocation(DoorRightClosedLocation + DoorRightOpenOffset * DoorAnimProgress);
}

void AWxElevator::SetAllInteractionsEnabled(bool bEnabled)
{
	PlatformInteraction->SetInteractionEnabled(bEnabled);
	CallConsoleAInteraction->SetInteractionEnabled(bEnabled);
	CallConsoleBInteraction->SetInteractionEnabled(bEnabled);
}

float AWxElevator::ComputeDoorWidth(const UStaticMeshComponent* DoorMesh) const
{
	const UStaticMesh* Mesh = DoorMesh ? DoorMesh->GetStaticMesh() : nullptr;
	if (!Mesh)
	{
		return 0.f;
	}

	return Mesh->GetBounds().BoxExtent.Y * 2.f * DoorMesh->GetRelativeScale3D().Y;
}
