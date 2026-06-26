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
	// 두 끝점(Start/End)을 잇는 열린 경로. Spline Move 는 각 상태가 가리키는 끝점 포인트 거리만 목표하므로 폐합 구간은 쓰지 않는다.
	SplineComponent->SetClosedLoop(false);

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

void AWxElevator::SetGimmickState(uint8 NewStateValue)
{
	// 베이스 CommitGimmickState(권위)가 호출하는 State 쓰기 훅. 라이브 이동/문 개폐는 ST_Elevator 가 State 변화를 Enum Compare 로 추종해 적용한다(위치는 Spline Move 가 전담).
	State = static_cast<EWxElevatorState>(NewStateValue);
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
		CommitGimmickState(static_cast<uint8>(EWxElevatorState::AtEnd));
	}
	else if (State == EWxElevatorState::AtEnd)
	{
		CommitGimmickState(static_cast<uint8>(EWxElevatorState::AtStart));
	}
}

void AWxElevator::HandleCallConsoleAInteracted(AActor* InteractingActor)
{
	if (!HasAuthority())
	{
		return;
	}

	// Start 호출. 이미 AtStart 면 동일값이라 복제 변화 없음(사실상 노옵).
	CommitGimmickState(static_cast<uint8>(EWxElevatorState::AtStart));
}

void AWxElevator::HandleCallConsoleBInteracted(AActor* InteractingActor)
{
	if (!HasAuthority())
	{
		return;
	}

	// End 호출. 이미 AtEnd 면 동일값이라 복제 변화 없음(사실상 노옵).
	CommitGimmickState(static_cast<uint8>(EWxElevatorState::AtEnd));
}
