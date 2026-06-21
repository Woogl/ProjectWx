// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxLaserCorridor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"

AWxLaserCorridor::AWxLaserCorridor()
{
	CorridorBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CorridorVolume"));
	CorridorBox->SetupAttachment(SceneRoot);
	CorridorBox->SetBoxExtent(FVector(1000.f, 250.f, 250.f));
	CorridorBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(Console);
}

void AWxLaserCorridor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxLaserCorridor, State);
}

void AWxLaserCorridor::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxLaserCorridor::HandleConsoleInteracted);
}

void AWxLaserCorridor::HandleConsoleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Disabled 로 확정한다. 스폰 중단·활성 레이저 제거·인터랙션 비활성은 ST 가 복제 State 를 추종해 적용한다.
	if (HasAuthority())
	{
		SetLaserCorridorState(EWxLaserCorridorState::Disabled);
	}
}

void AWxLaserCorridor::SetLaserCorridorState(EWxLaserCorridorState NewState)
{
	// State 쓰기는 권위 전용. 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종한다.
	if (!HasAuthority() || State == NewState)
	{
		return;
	}

	State = NewState;
}
