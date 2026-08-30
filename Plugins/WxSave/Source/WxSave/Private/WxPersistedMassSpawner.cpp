// Copyright Woogle. All Rights Reserved.

#include "WxPersistedMassSpawner.h"

#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassEntityTemplate.h"
#include "MassSpawnerTypes.h"
#include "WxPersistableMassTrait.h"
#include "WxSaveModule.h"

AWxPersistedMassSpawner::AWxPersistedMassSpawner()
{
	bAutoSpawnOnBeginPlay = false;
}

void AWxPersistedMassSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (ShouldSpawnEntities())
	{
		OnSpawningFinishedEvent.AddDynamic(this, &AWxPersistedMassSpawner::HandleSpawningFinished);
		DoSpawning();
	}
}

void AWxPersistedMassSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AllSpawnedEntities.Empty();
	Super::EndPlay(EndPlayReason);
}

bool AWxPersistedMassSpawner::ShouldSpawnEntities() const
{
	return !bHasEverSpawned;
}

void AWxPersistedMassSpawner::HandleSpawningFinished()
{
	OnSpawningFinishedEvent.RemoveDynamic(this, &AWxPersistedMassSpawner::HandleSpawningFinished);
	bHasEverSpawned = true;
	StampOriginFragmentOnSpawnedEntities();
}

void AWxPersistedMassSpawner::StampOriginFragmentOnSpawnedEntities()
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
	for (const FSpawnedEntities& Spawned : AllSpawnedEntities)
	{
		UMassEntityConfigAsset* ResolvedConfig = nullptr;
		for (const FMassSpawnedEntityType& EntityType : EntityTypes)
		{
			UMassEntityConfigAsset* Config = const_cast<UMassEntityConfigAsset*>(EntityType.GetEntityConfig());
			if (Config && Config->GetOrCreateEntityTemplate(*GetWorld()).GetTemplateID() == Spawned.TemplateID)
			{
				ResolvedConfig = Config;
				break;
			}
		}

		if (!ResolvedConfig)
		{
			UE_LOG(LogWxSave, Warning, TEXT("%s: Mass template %s의 EntityConfig를 찾지 못해 %d개 엔티티를 영속화 대상으로 표시하지 못했다."),
				*GetName(), *Spawned.TemplateID.ToString(), Spawned.Entities.Num());
			continue;
		}

		for (FMassEntityHandle Entity : Spawned.Entities)
		{
			if (FWxPersistableEntityConfigFragment* Origin = EntityManager.GetFragmentDataPtr<FWxPersistableEntityConfigFragment>(Entity))
			{
				Origin->EntityConfig = ResolvedConfig;
			}
		}
	}
}
