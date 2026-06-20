// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxDoor.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"

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
}

void AWxDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxDoor, State);
}

void AWxDoor::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxDoor::HandleConsoleInteracted);
}

void AWxDoor::SetDoorState(EWxDoorState NewState)
{
	// State 쓰기는 권위 전용. 클라는 복제로 동기화된다. 전이는 ST_Door 의 Enum Compare 전이 조건이 State 변화를 감지해 구동한다(서버/클라 동일).
	if (!HasAuthority() || State == NewState)
	{
		return;
	}

	State = NewState;
}

void AWxDoor::HandleConsoleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 처리하며, 현재 상태의 반대 목표로 확정한다(슬라이드는 StateTree 의 Wx Component Move 가 비주얼로 처리).
	// 닫기(Open→Close)는 준비돼 있으나, Open 상태의 인터랙션 태스크가 에셋에서 비활성이라 Open 에선 이 콜백이 호출되지 않아 현재 단방향으로 동작한다.
	// 클라이언트는 복제된 State 를 Enum Compare 전이가 감지해 동일 전이하므로 비권위 분기는 노옵.
	if (!HasAuthority())
	{
		return;
	}

	if (State == EWxDoorState::Close)
	{
		SetDoorState(EWxDoorState::Open);
	}
	else if (State == EWxDoorState::Open)
	{
		SetDoorState(EWxDoorState::Close);
	}
}
