// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxAlarmConsole.h"

#include "Components/StateTreeComponent.h"
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

	// 모든 컴포넌트의 BeginPlay 가 끝난 뒤 StateTree 를 시작한다(인터랙션 바인딩 후).
	StateTree->StartLogic();
}

void AWxAlarmConsole::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Alarmed 로 확정한다. FX·인터랙션 비활성은 ST 의 Alarmed 상태(Wx Play Fx / Wx Gimmick Interaction)가 복제 State 를 추종해 적용한다.
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
