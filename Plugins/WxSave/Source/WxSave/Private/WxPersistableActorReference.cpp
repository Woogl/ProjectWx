// Copyright Woogle. All Rights Reserved.

#include "WxPersistableActorReference.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WxPersistableActorReferenceManager.h"

bool FWxPersistableActorReference::Capture(const AActor* Actor)
{
	Type = EWxPersistableActorReferenceType::None;
	LevelPath.Reset();
	ActorName = NAME_None;
	PlayerIndex = INDEX_NONE;

	if (!Actor)
	{
		return false;
	}

	UWorld* World = Actor->GetWorld();
	if (!World)
	{
		return false;
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(Actor))
	{
		const int32 NumControllers = UGameplayStatics::GetNumPlayerControllers(World);
		for (int32 Index = 0; Index < NumControllers; ++Index)
		{
			if (UGameplayStatics::GetPlayerController(World, Index) == PlayerController)
			{
				Type = EWxPersistableActorReferenceType::PlayerController;
				PlayerIndex = Index;
				return true;
			}
		}
	}

	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		const int32 NumControllers = UGameplayStatics::GetNumPlayerControllers(World);
		for (int32 Index = 0; Index < NumControllers; ++Index)
		{
			if (UGameplayStatics::GetPlayerPawn(World, Index) == Pawn)
			{
				Type = EWxPersistableActorReferenceType::PlayerPawn;
				PlayerIndex = Index;
				return true;
			}
		}
	}

	const ULevel* Level = Actor->GetLevel();
	if (Level)
	{
		Type = EWxPersistableActorReferenceType::LevelActor;
		LevelPath = FSoftObjectPath(Level);
		ActorName = Actor->GetFName();
		return true;
	}

	return false;
}

AActor* FWxPersistableActorReference::Resolve(UWorld* World) const
{
	switch (Type)
	{
	case EWxPersistableActorReferenceType::PlayerController:
		return UGameplayStatics::GetPlayerController(World, PlayerIndex);

	case EWxPersistableActorReferenceType::PlayerPawn:
		return UGameplayStatics::GetPlayerPawn(World, PlayerIndex);

	case EWxPersistableActorReferenceType::LevelActor:
		if (UWxPersistableActorReferenceManager* Manager = World
			? World->GetSubsystem<UWxPersistableActorReferenceManager>()
			: nullptr)
		{
			return Manager->GetPersistedRuntimeActor(LevelPath, ActorName);
		}
		return nullptr;

	case EWxPersistableActorReferenceType::None:
	default:
		return nullptr;
	}
}

bool FWxPersistableActorReference::IsSet() const
{
	return Type != EWxPersistableActorReferenceType::None;
}
