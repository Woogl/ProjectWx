// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxSpawnConsole.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Spawnable/WxSpawner.h"

AWxSpawnConsole::AWxSpawnConsole()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ConsoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConsoleMesh"));
	ConsoleMesh->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(ConsoleMesh);
}

void AWxSpawnConsole::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxSpawnConsole::HandleInteracted);

	ApplyState();
}

void AWxSpawnConsole::ApplyState()
{
	if (bTriggered)
	{
		ConsoleInteraction->SetInteractionEnabled(false);
	}
}

void AWxSpawnConsole::HandleInteracted(AActor* InstigatorActor)
{
	if (!HasAuthority() || bTriggered)
	{
		return;
	}

	MarkTriggered();

	// 스트리밍 아웃된 Spawner 는 강제 로드하지 않고 스킵. 디자이너가 콘솔과 같은 영역에 배치되도록 보장해야 함.
	for (const TSoftObjectPtr<AWxSpawner>& SoftSpawner : TargetSpawners)
	{
		if (AWxSpawner* Spawner = SoftSpawner.Get())
		{
			Spawner->Respawn();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AWxSpawnConsole '%s': TargetSpawner is null or not loaded."), *GetName());
		}
	}
}
