// Copyright Woogle. All Rights Reserved.

#include "WxPersistableReferencedActorComponent.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "WxPersistableActorReferenceManager.h"

UWxPersistableReferencedActorComponent::UWxPersistableReferencedActorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxPersistableReferencedActorComponent::OnSavePreparing()
{
	AActor* Owner = GetOwner();
	ULevel* Level = Owner ? Owner->GetLevel() : nullptr;
	if (!Owner || !Level)
	{
		return;
	}

	LastSessionLevelPath = FSoftObjectPath(Level);
	LastSessionActorName = Owner->GetFName();
}

void UWxPersistableReferencedActorComponent::OnSaveRestored(const TArray<FName>& RestoredPropertyNames)
{
	static_cast<void>(RestoredPropertyNames);

	if (LastSessionLevelPath.IsNull() || LastSessionActorName.IsNone())
	{
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	UWxPersistableActorReferenceManager* Manager = World
		? World->GetSubsystem<UWxPersistableActorReferenceManager>()
		: nullptr;
	if (!Manager)
	{
		return;
	}

	Manager->RegisterActor(LastSessionLevelPath, LastSessionActorName, Owner);
	bRegisteredWithManager = true;
}

void UWxPersistableReferencedActorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bRegisteredWithManager)
	{
		if (UWorld* World = GetWorld())
		{
			if (UWxPersistableActorReferenceManager* Manager =
				World->GetSubsystem<UWxPersistableActorReferenceManager>())
			{
				Manager->UnregisterActor(LastSessionLevelPath, LastSessionActorName);
			}
		}
		bRegisteredWithManager = false;
	}

	Super::EndPlay(EndPlayReason);
}
