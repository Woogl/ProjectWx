// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxAlarmConsole.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"

AWxAlarmConsole::AWxAlarmConsole()
{
	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(Console);
}

void AWxAlarmConsole::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxAlarmConsole, State);
}

void AWxAlarmConsole::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxAlarmConsole::HandleInteracted);
}

void AWxAlarmConsole::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Alarmed 로 확정한다. FX·인터랙션 비활성은 ST 의 Alarmed 상태(Wx Spawn Niagara / Wx Play Sound / Wx Enable Interaction)가 복제 State 를 추종해 적용한다.
	if (HasAuthority())
	{
		SetAlarmConsoleState(EWxAlarmConsoleState::Alarmed);
	}
}

void AWxAlarmConsole::SetAlarmConsoleState(EWxAlarmConsoleState NewState)
{
	// State 쓰기는 권위 전용. 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종한다.
	if (!HasAuthority() || State == NewState)
	{
		return;
	}

	State = NewState;
}
