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

void AWxAlarmConsole::SetGimmickState(uint8 NewStateValue)
{
	// 베이스 CommitGimmickState(권위)가 호출하는 State 쓰기 훅. FX·인터랙션 비활성은 ST 가 State 변화를 Enum Compare 로 추종해 적용한다.
	State = static_cast<EWxAlarmConsoleState>(NewStateValue);
}

void AWxAlarmConsole::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Alarmed 로 확정한다. 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종하므로 비권위는 노옵.
	if (HasAuthority())
	{
		CommitGimmickState(static_cast<uint8>(EWxAlarmConsoleState::Alarmed));
	}
}
