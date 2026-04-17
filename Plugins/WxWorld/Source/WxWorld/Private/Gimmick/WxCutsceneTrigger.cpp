// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxCutsceneTrigger.h"

#include "Component/WxInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/WxInteractionWidgetComponent.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

AWxCutsceneTrigger::AWxCutsceneTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	bIsPlaying = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InteractionWidget = CreateDefaultSubobject<UWxInteractionWidgetComponent>(TEXT("InteractionWidget"));
	InteractionWidget->SetupAttachment(MeshComponent);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
	InteractionComponent->InteractionWidget = InteractionWidget;
}

void AWxCutsceneTrigger::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionComponent)
	{
		InteractionComponent->OnInteracted.AddDynamic(this, &AWxCutsceneTrigger::HandleInteracted);
	}
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

	if (InteractionComponent)
	{
		InteractionComponent->SetInteractionEnabled(false);
	}

	SequencePlayer->OnFinished.AddDynamic(this, &AWxCutsceneTrigger::HandleSequenceFinished);
	SequencePlayer->Play();
}

void AWxCutsceneTrigger::HandleSequenceFinished()
{
	CleanupSequenceActor();
	bIsPlaying = false;

	if (InteractionComponent)
	{
		InteractionComponent->SetInteractionEnabled(true);
	}
}

void AWxCutsceneTrigger::CleanupSequenceActor()
{
	if (SequenceActor)
	{
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}
}

