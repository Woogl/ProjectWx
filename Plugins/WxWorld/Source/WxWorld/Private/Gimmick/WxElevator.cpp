// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxElevator.h"

#include "Components/SplineComponent.h"
#include "Components/StateTreeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"

AWxElevator::AWxElevator()
{
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

	ElevatorStateTree = CreateDefaultSubobject<UStateTreeComponent>(TEXT("ElevatorStateTree"));
	// 모든 컴포넌트의 BeginPlay 이후 BeginPlay 에서 직접 시작한다(인터랙션 토글 등 컴포넌트 의존 순서 보장).
	ElevatorStateTree->SetStartLogicAutomatically(false);
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
	DoorLeftClosedLocation = DoorLeft->GetRelativeLocation();
	DoorRightClosedLocation = DoorRight->GetRelativeLocation();

	// 각 문은 자기 메시 너비만큼 바깥쪽(좌: -Y, 우: +Y)으로 슬라이드.
	DoorLeftOpenOffset = FVector(0.f, -ComputeDoorWidth(DoorLeft), 0.f);
	DoorRightOpenOffset = FVector(0.f, ComputeDoorWidth(DoorRight), 0.f);

	PlatformInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandlePlatformInteracted);
	CallConsoleAInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandleCallConsoleAInteracted);
	CallConsoleBInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandleCallConsoleBInteracted);

	// StartLogic 전에 현재 State 에 맞는 위치/문 알파로 미리 스냅한다(Level Streaming Persistence / 슬롯 복원).
	// 이후 StateTree 의 초기 선택이 ElevatorStateIs 로 현재 State 에 맞는 상태를 잡아 인터랙션·애니를 잇는다.
	SnapVisualsToState();

	// 모든 컴포넌트의 BeginPlay 가 끝난 뒤 StateTree 를 시작한다.
	ElevatorStateTree->StartLogic();
}

void AWxElevator::ApplyState()
{
	// 매 상태 적용 시 위치/문 알파를 현재 State 에 맞게 스냅한다(복원·복제 정합).
	// 전이는 별도 통지 없이 전이 태스크(ElevatorMove/ElevatorDoorPose)가 State 변화를 감지해 구동한다.
	// 서버(SetElevatorState)·클라(OnRep)·복원(OnWxSaveRestored) 모두 이 한 경로로 위치/알파를 스냅한다.
	SnapVisualsToState();
}

void AWxElevator::SnapVisualsToState()
{
	// TargetEndpoint = 이 시퀀스의 최종 목적지. 정지 상태(Closed/Open) 와 도착 직후(DoorsOpening) 에선 현재 위치 = TargetEndpoint.
	// 반면 DoorsClosing (출발 직전 정지) 과 Moving (이동 중) 에선 출발 끝점 = TargetEndpoint 의 반대.
	const float TargetEndpointDistance = TargetEndpoint == EWxElevatorEndpoint::End ? CachedSplineLength : 0.f;
	const float OppositeEndpointDistance = TargetEndpoint == EWxElevatorEndpoint::End ? 0.f : CachedSplineLength;

	TargetDistance = TargetEndpointDistance;

	float NewDistance = CurrentDistance;
	float NewAlpha = CurrentOpenAlpha;

	switch (State)
	{
	case EWxElevatorState::DoorsClosed:
		NewDistance = TargetEndpointDistance;
		NewAlpha = 0.f;
		break;

	case EWxElevatorState::DoorsOpening:
		// 도착 직후이거나 슬롯 복원 — 둘 다 목적지 끝점으로 스냅. 알파는 진입 직전 값(0)을 유지하고 태스크가 1까지 올린다.
		NewDistance = TargetEndpointDistance;
		break;

	case EWxElevatorState::DoorsOpen:
		NewDistance = TargetEndpointDistance;
		NewAlpha = 1.f;
		break;

	case EWxElevatorState::DoorsClosing:
		// 출발 끝점(목적지의 반대) 에 정지한 채 문 닫는 중. 알파 1 로 스냅하고 태스크가 0까지 내린다.
		NewDistance = OppositeEndpointDistance;
		NewAlpha = 1.f;
		break;

	case EWxElevatorState::Moving:
		// 슬롯 복원 보정: CurrentDistance 가 디폴트(0)라 출발/목적이 같으면 반대 끝점에서 출발하도록 강제.
		// 정상 흐름의 BeginMoveSequence 는 CurrentDistance 가 현재 위치를 유지하므로 이 분기에 들어가지 않는다.
		if (FMath::IsNearlyEqual(CurrentDistance, TargetEndpointDistance))
		{
			NewDistance = OppositeEndpointDistance;
		}
		NewAlpha = 0.f;
		break;
	}

	SetPlatformDistance(NewDistance);
	SetDoorOpenAlpha(NewAlpha);
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
			SetElevatorState(EWxElevatorState::DoorsOpening);
		}
		return;
	}

	TargetEndpoint = NewEndpoint;
	TargetDistance = NewTargetDistance;

	if (State == EWxElevatorState::DoorsOpen)
	{
		SetElevatorState(EWxElevatorState::DoorsClosing);
	}
	else
	{
		// 문이 이미 닫혀 있으므로 DoorsClosing 단계 건너뛰고 즉시 이동 시작.
		SetElevatorState(EWxElevatorState::Moving);
	}
}

void AWxElevator::SetElevatorState(EWxElevatorState NewState)
{
	// State 쓰기는 권위 전용. 클라는 OnRep_State 로 동기화된다.
	if (!HasAuthority() || State == NewState)
	{
		return;
	}

	State = NewState;
	ApplyState();
}

void AWxElevator::SetPlatformDistance(float Distance)
{
	CurrentDistance = FMath::Clamp(Distance, 0.f, CachedSplineLength);
	const FVector NewLocation = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::Local);
	PlatformRoot->SetRelativeLocation(NewLocation);
}

void AWxElevator::SetDoorOpenAlpha(float Alpha)
{
	CurrentOpenAlpha = FMath::Clamp(Alpha, 0.f, 1.f);
	DoorLeft->SetRelativeLocation(DoorLeftClosedLocation + DoorLeftOpenOffset * CurrentOpenAlpha);
	DoorRight->SetRelativeLocation(DoorRightClosedLocation + DoorRightOpenOffset * CurrentOpenAlpha);
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
