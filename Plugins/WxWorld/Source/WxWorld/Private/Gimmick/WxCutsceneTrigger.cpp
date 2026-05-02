// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxCutsceneTrigger.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionWidgetComponent.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

AWxCutsceneTrigger::AWxCutsceneTrigger()
{
	bIsPlaying = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InteractionWidget->SetupAttachment(MeshComponent);
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxCutsceneTrigger::BeginPlay()
{
	Super::BeginPlay();

	OnInteracted.AddDynamic(this, &AWxCutsceneTrigger::HandleInteracted);
}

void AWxCutsceneTrigger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupSequenceActor();

	Super::EndPlay(EndPlayReason);
}

void AWxCutsceneTrigger::HandleInteracted(AActor* InteractingActor)
{
	if (bIsPlaying || !LevelSequence)
	{
		return;
	}

	bIsPlaying = true;

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* NewSequenceActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), LevelSequence, PlaybackSettings, NewSequenceActor);
	SequenceActor = NewSequenceActor;

	if (!SequencePlayer)
	{
		CleanupSequenceActor();
		bIsPlaying = false;
		return;
	}

	SetInteractionEnabled(false);

	SequencePlayer->OnFinished.AddDynamic(this, &AWxCutsceneTrigger::HandleSequenceFinished);
	SequencePlayer->Play();
}

void AWxCutsceneTrigger::HandleSequenceFinished()
{
	CleanupSequenceActor();
	bIsPlaying = false;

	SetInteractionEnabled(true);
}

void AWxCutsceneTrigger::CleanupSequenceActor()
{
	if (SequenceActor)
	{
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}
}
