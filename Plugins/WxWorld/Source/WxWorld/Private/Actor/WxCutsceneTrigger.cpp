// Copyright Woogle. All Rights Reserved.

#include "Actor/WxCutsceneTrigger.h"

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

	SequencePlayer->OnFinished.AddDynamic(this, &AWxCutsceneTrigger::HandleSequenceFinished);
	SequencePlayer->Play();
}

void AWxCutsceneTrigger::HandleSequenceFinished()
{
	CleanupSequenceActor();
	bIsPlaying = false;
}

void AWxCutsceneTrigger::CleanupSequenceActor()
{
	if (SequenceActor)
	{
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}
}

#if WITH_EDITOR
UStreamableRenderAsset* AWxCutsceneTrigger::GetEditorPreviewMesh() const
{
	return MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
}
#endif
