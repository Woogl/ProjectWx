// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxDoor.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"

AWxDoor::AWxDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	Door->SetupAttachment(SceneRoot);

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

	// 문 닫힘 위치 캐시: BP/레벨에서 배치된 상대 위치를 닫힘 기준으로 사용.
	DoorClosedLocation = Door->GetRelativeLocation();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxDoor::HandleConsoleInteracted);

	// Level Streaming Persistence 로 State 가 BeginPlay 직전에 직접 set 되므로 OnRep 이 발화하지 않는다.
	// 영구화된 State 를 시각/인터랙션에 반영하기 위해 ApplyState 를 명시 호출.
	ApplyState();
}

void AWxDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (State != EWxDoorState::Opening)
	{
		return;
	}

	const float Step = DoorAnimDuration > 0.f ? DeltaTime / DoorAnimDuration : 1.f;
	DoorAnimProgress = FMath::Clamp(DoorAnimProgress + Step, 0.f, 1.f);
	UpdateDoorPosition();

	if (HasAuthority() && DoorAnimProgress >= 1.f)
	{
		State = EWxDoorState::Open;
		ApplyState();
	}
}

void AWxDoor::HandleConsoleInteracted(AActor* InstigatorActor)
{
	if (!HasAuthority() || State != EWxDoorState::Closed)
	{
		return;
	}

	State = EWxDoorState::Opening;
	ApplyState();
}

void AWxDoor::OnRep_State()
{
	ApplyState();
}

void AWxDoor::ApplyState()
{
	switch (State)
	{
	case EWxDoorState::Closed:
		SetActorTickEnabled(false);
		ConsoleInteraction->SetInteractionEnabled(true);
		DoorAnimProgress = 0.f;
		UpdateDoorPosition();
		break;

	case EWxDoorState::Opening:
		SetActorTickEnabled(true);
		ConsoleInteraction->SetInteractionEnabled(false);
		// DoorAnimProgress 는 진입 직전 값(Closed 에서 0)을 그대로 사용; Tick 이 1까지 증가시킴.
		break;

	case EWxDoorState::Open:
		SetActorTickEnabled(false);
		// 한 번 열린 문은 영구적으로 상호작용 비활성.
		ConsoleInteraction->SetInteractionEnabled(false);
		DoorAnimProgress = 1.f;
		UpdateDoorPosition();
		break;
	}
}

void AWxDoor::UpdateDoorPosition()
{
	const FVector Offset = DoorOpenOffset * DoorAnimProgress;

	Door->SetRelativeLocation(DoorClosedLocation + Offset);
}
