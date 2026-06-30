// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxAlarmConsole.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "WxGameplayTags.h"

AWxAlarmConsole::AWxAlarmConsole()
{
	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(Console);
	ConsoleInteraction->SetHighlightTarget(Console);

	State = WxGameplayTags::Gimmick_AlarmConsole_Idle;
}

void AWxAlarmConsole::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxAlarmConsole::HandleInteracted);
}

void AWxAlarmConsole::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Alarmed 로 확정한다. 클라는 복제 State 의 OnRep 이벤트가 ST 진입을 구동하므로 비권위는 노옵.
	if (HasAuthority())
	{
		CommitGimmickState(WxGameplayTags::Gimmick_AlarmConsole_Alarmed);
	}
}
