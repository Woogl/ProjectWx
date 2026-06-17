// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxDoor.h"

#include "Components/StateTreeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"
#include "WxGameplayTags.h"

AWxDoor::AWxDoor()
{
	DoorLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorLeft"));
	DoorLeft->SetupAttachment(SceneRoot);

	DoorRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorRight"));
	DoorRight->SetupAttachment(SceneRoot);

	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(Console);

	DoorStateTree = CreateDefaultSubobject<UStateTreeComponent>(TEXT("DoorStateTree"));
	// 모든 컴포넌트의 BeginPlay 이후 BeginPlay 에서 직접 시작한다(인터랙션 토글 등 컴포넌트 의존 순서 보장).
	DoorStateTree->SetStartLogicAutomatically(false);
}

void AWxDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxDoor, State);
}

void AWxDoor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 오프셋을 BeginPlay 보다 앞선 시점에 캐시한다. Super::BeginPlay → StartLogic 이
	// 복원 문을 Open 포즈로 스냅할 때 오프셋이 이미 준비되어 있어야 한다.
	CacheDoorPoseAnchors();
}

void AWxDoor::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxDoor::HandleConsoleInteracted);

	// StartLogic 전에 현재 State 에 맞는 포즈로 미리 스냅한다. 이후 DoorPose 가 현재→목표로 보간하므로,
	// 복원/레이트조인으로 이미 열린(Open)·닫는중(Closing) 문은 현재=목표가 되어 애니 없이 스냅된다.
	const bool bStartOpen = (State == EWxDoorState::Open || State == EWxDoorState::Closing);
	SetDoorOpenAlpha(bStartOpen ? 1.f : 0.f);

	// 모든 컴포넌트의 BeginPlay 가 끝난 뒤 StateTree 를 시작한다.
	// 시작 시 DoorStateIs 조건이 현재 State 를 읽어 맞는 상태로 초기 선택한다.
	DoorStateTree->StartLogic();
}

void AWxDoor::ApplyState()
{
	// State 변경을 StateTree 에 알려 현재 State 에 맞는 상태로 Root 재선택을 유발한다.
	// 서버(SetDoorState)·클라(OnRep_State)·복원(OnWxSaveRestored) 모두 이 한 경로로 StateTree 를 동기화한다.
	if (DoorStateTree && DoorStateTree->IsRunning())
	{
		DoorStateTree->SendStateTreeEvent(WxGameplayTags::Event_Gimmick_StateChanged);
	}
}

void AWxDoor::SetDoorState(EWxDoorState NewState)
{
	// State 쓰기는 권위 전용. 클라는 OnRep_State 로 동기화된다.
	if (!HasAuthority() || State == NewState)
	{
		return;
	}

	State = NewState;
	ApplyState();
}

void AWxDoor::HandleConsoleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 처리하며, 현재 상태에 따라 개방/닫기를 시작한다.
	// 닫기(Open→Closing)는 준비돼 있으나, Open 의 DoorPose interaction 이 에셋에서 비활성이라 Open 에선 이 콜백이 호출되지 않아 현재 단방향으로 동작한다.
	// Open interaction 을 켜면 반복 개폐가 활성화된다.
	// 클라이언트는 OnRep_State → ApplyState 로 동일 처리되므로 비권위 분기는 노옵.
	if (!HasAuthority())
	{
		return;
	}

	if (State == EWxDoorState::Closed)
	{
		SetDoorState(EWxDoorState::Opening);
	}
	else if (State == EWxDoorState::Open)
	{
		SetDoorState(EWxDoorState::Closing);
	}
}

void AWxDoor::OnRep_State()
{
	ApplyState();
}

void AWxDoor::SetConsoleInteractionEnabled(bool bEnabled)
{
	ConsoleInteraction->SetInteractionEnabled(bEnabled);
}

void AWxDoor::SetDoorOpenAlpha(float Alpha)
{
	const float Clamped = FMath::Clamp(Alpha, 0.f, 1.f);
	DoorLeft->SetRelativeLocation(DoorLeftClosedLocation + DoorLeftOpenOffset * Clamped);
	DoorRight->SetRelativeLocation(DoorRightClosedLocation + DoorRightOpenOffset * Clamped);
	CurrentOpenAlpha = Clamped;
}

void AWxDoor::CacheDoorPoseAnchors()
{
	// 문 닫힘 위치 캐시: BP/레벨에서 배치된 상대 위치를 닫힘 기준으로 사용.
	DoorLeftClosedLocation = DoorLeft->GetRelativeLocation();
	DoorRightClosedLocation = DoorRight->GetRelativeLocation();

	// 각 문은 자기 메시 너비만큼 바깥쪽(좌: -Y, 우: +Y)으로 슬라이드.
	DoorLeftOpenOffset = FVector(0.f, -ComputeDoorWidth(DoorLeft), 0.f);
	DoorRightOpenOffset = FVector(0.f, ComputeDoorWidth(DoorRight), 0.f);
}

float AWxDoor::ComputeDoorWidth(const UStaticMeshComponent* DoorMesh) const
{
	const UStaticMesh* Mesh = DoorMesh ? DoorMesh->GetStaticMesh() : nullptr;
	if (!Mesh)
	{
		return 0.f;
	}

	return Mesh->GetBounds().BoxExtent.Y * 2.f * DoorMesh->GetRelativeScale3D().Y;
}
