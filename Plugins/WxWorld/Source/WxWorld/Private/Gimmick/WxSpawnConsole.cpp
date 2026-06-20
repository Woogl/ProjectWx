// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxSpawnConsole.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"

AWxSpawnConsole::AWxSpawnConsole()
{
	ConsoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConsoleMesh"));
	ConsoleMesh->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(ConsoleMesh);
}

void AWxSpawnConsole::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxSpawnConsole, State);
}

void AWxSpawnConsole::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxSpawnConsole::HandleInteracted);
}

void AWxSpawnConsole::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Spawned 로 확정한다. 스포너 Respawn·인터랙션 비활성은 ST 의 Spawned 상태(Wx Trigger Spawners / Wx Gimmick Interaction)가 복제 State 를 추종해 적용한다.
	if (HasAuthority())
	{
		SetSpawnConsoleState(EWxSpawnConsoleState::Spawned);
	}
}

void AWxSpawnConsole::SetSpawnConsoleState(EWxSpawnConsoleState NewState)
{
	// State 쓰기는 권위 전용. 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종한다.
	if (!HasAuthority() || State == NewState)
	{
		return;
	}

	State = NewState;
}
