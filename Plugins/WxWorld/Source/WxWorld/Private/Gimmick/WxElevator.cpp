// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxElevator.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"

AWxElevator::AWxElevator()
{
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(SceneRoot);
	// 두 끝점(Start/End)을 잇는 경로. Spline Move 는 각 상태가 가리키는 끝점 포인트 거리만 목표하므로 닫힘/열림 형상과 무관하나, 기존 에셋 스플라인 형상을 보존하기 위해 닫힌 루프를 유지한다.
	SplineComponent->SetClosedLoop(true);

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
}

void AWxElevator::BeginPlay()
{
	Super::BeginPlay();

	PlatformInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandlePlatformInteracted);
	CallConsoleAInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandleCallConsoleAInteracted);
	CallConsoleBInteraction->OnInteracted.AddDynamic(this, &AWxElevator::HandleCallConsoleBInteracted);
}

void AWxElevator::HandlePlatformInteracted(AActor* InteractingActor)
{
	if (!HasAuthority())
	{
		return;
	}

	// 플랫폼 위 상호작용은 반대 끝점으로 토글한다(문이 열린 정지 상태에서만 — Closed 는 문 닫힘이라 플랫폼에 탈 수 없다).
	if (State == EWxElevatorState::AtStart)
	{
		SetElevatorState(EWxElevatorState::AtEnd);
	}
	else if (State == EWxElevatorState::AtEnd)
	{
		SetElevatorState(EWxElevatorState::AtStart);
	}
}

void AWxElevator::HandleCallConsoleAInteracted(AActor* InteractingActor)
{
	if (!HasAuthority())
	{
		return;
	}

	// Start 호출. 이미 AtStart 면 SetElevatorState 가 노옵 처리.
	SetElevatorState(EWxElevatorState::AtStart);
}

void AWxElevator::HandleCallConsoleBInteracted(AActor* InteractingActor)
{
	if (!HasAuthority())
	{
		return;
	}

	// End 호출. 이미 AtEnd 면 SetElevatorState 가 노옵 처리.
	SetElevatorState(EWxElevatorState::AtEnd);
}

void AWxElevator::SetElevatorState(EWxElevatorState NewState)
{
	// State 쓰기는 권위 전용. 클라는 복제 State 를 ST_Elevator 의 Enum Compare 전이가 추종한다.
	// 라이브 이동은 StateTree(Spline Move)가 구동하므로 여기서 위치 스냅을 하지 않는다(하면 도착지로 순간이동).
	if (!HasAuthority() || State == NewState)
	{
		return;
	}

	State = NewState;
}
