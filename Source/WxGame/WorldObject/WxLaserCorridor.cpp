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

void AWxLaserCorridor::SetGimmickState(uint8 NewStateValue)
{
	// 베이스 CommitGimmickState(권위)가 호출하는 State 쓰기 훅. 스폰 중단·레이저 철거·인터랙션 비활성은 ST 가 State 변화를 Enum Compare 로 추종해 적용한다.
	State = static_cast<EWxLaserCorridorState>(NewStateValue);
}

void AWxLaserCorridor::HandleConsoleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Disabled 로 확정한다. 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종하므로 비권위는 노옵.
	if (HasAuthority())
	{
		CommitGimmickState(static_cast<uint8>(EWxLaserCorridorState::Disabled));
	}
}
