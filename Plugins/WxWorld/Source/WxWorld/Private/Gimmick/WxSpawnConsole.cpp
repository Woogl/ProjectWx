// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxSpawnConsole.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "WxGameplayTags.h"

AWxSpawnConsole::AWxSpawnConsole()
{
	ConsoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConsoleMesh"));
	ConsoleMesh->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(ConsoleMesh);

	State = WxGameplayTags::Gimmick_SpawnConsole_Idle;
}

void AWxSpawnConsole::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxSpawnConsole::HandleInteracted);
}

void AWxSpawnConsole::HandleInteracted(AActor* InstigatorActor)
{
	// 권위 측만 State 를 Spawned 로 확정한다. 클라는 복제 State 의 OnRep 이벤트가 ST 진입을 구동하므로 비권위는 노옵.
	if (HasAuthority())
	{
		CommitGimmickState(WxGameplayTags::Gimmick_SpawnConsole_Spawned);
	}
}
