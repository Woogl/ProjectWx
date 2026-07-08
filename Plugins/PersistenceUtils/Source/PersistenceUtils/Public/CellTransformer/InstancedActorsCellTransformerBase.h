// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "WorldPartition/WorldPartitionRuntimeCellTransformer.h"
#include "InstancedActorsCellTransformerBase.generated.h"

class AActor;
class AInstancedActorsManager;
class ULevel;

DECLARE_LOG_CATEGORY_EXTERN(LogInstancedActorsCellTransformer, Log, All);

/**
 * World Partition Runtime Cell Transformer that converts placed actors into Instanced Actors at cook time.
 *
 * For each streaming cell, any actor whose class derives from an entry in ActorClassesToConvert and which passes
 * ShouldConvertActorToInstanced() is collected, removed, and re-added as an instance on a single
 * AInstancedActorsManager spawned into the cell.
 */
UCLASS(MinimalAPI, EditInlineNew)
class UInstancedActorsCellTransformerBase : public UWorldPartitionRuntimeCellTransformer
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	//~ Begin UWorldPartitionRuntimeCellTransformer Interface
	PERSISTENCEUTILS_API virtual void Transform(ULevel* InLevel) override;
	//~ End UWorldPartitionRuntimeCellTransformer Interface

protected:
	/**
	 * Per-actor predicate deciding whether an actor (already matched against ActorClassesToConvert) should be
	 * converted. Override to add project-specific filtering. The base implementation always returns true.
	 */
	PERSISTENCEUTILS_API virtual bool ShouldConvertActorToInstanced(const AActor* Actor) const;

	/** The Instanced Actors manager class to spawn. Override to provide a project-specific manager. */
	PERSISTENCEUTILS_API virtual TSubclassOf<AInstancedActorsManager> GetInstancedActorsManagerClass() const;
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
protected:
	/** Actors whose class derives from one of these are candidates for conversion to Instanced Actors. */
	UPROPERTY(EditAnywhere, Category = "Instanced Actors")
	TArray<TSubclassOf<AActor>> ActorClassesToConvert;
#endif // WITH_EDITORONLY_DATA
};
