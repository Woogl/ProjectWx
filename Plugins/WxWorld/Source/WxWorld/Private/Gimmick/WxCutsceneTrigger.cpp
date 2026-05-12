// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxCutsceneTrigger.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WxInteractionComponent.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

AWxCutsceneTrigger::AWxCutsceneTrigger()
{
	bIsPlaying = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxCutsceneTrigger::BeginPlay()
{
	Super::BeginPlay();

	InteractionComponent->OnInteracted.AddDynamic(this, &AWxCutsceneTrigger::HandleInteracted);
}

void AWxCutsceneTrigger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 컷신 도중 트리거가 파괴되면 입력 락을 풀어줘야 한다.
	if (bIsPlaying)
	{
		SetLocalPlayerInputEnabled(true);
	}

	CleanupSequenceActor();

	Super::EndPlay(EndPlayReason);
}

void AWxCutsceneTrigger::HandleInteracted(AActor* InstigatorActor)
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

	InteractionComponent->SetInteractionEnabled(false);
	SetLocalPlayerInputEnabled(false);

	SequencePlayer->OnFinished.AddDynamic(this, &AWxCutsceneTrigger::HandleSequenceFinished);
	SequencePlayer->Play();
}

void AWxCutsceneTrigger::HandleSequenceFinished()
{
	CleanupSequenceActor();
	bIsPlaying = false;

	InteractionComponent->SetInteractionEnabled(true);
	SetLocalPlayerInputEnabled(true);
}

void AWxCutsceneTrigger::CleanupSequenceActor()
{
	if (SequenceActor)
	{
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}
}

void AWxCutsceneTrigger::SetLocalPlayerInputEnabled(bool bEnabled)
{
	APlayerController* PC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return;
	}

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
	{
		return;
	}

	if (bEnabled)
	{
		Pawn->EnableInput(PC);
	}
	else
	{
		Pawn->DisableInput(PC);
	}
}
