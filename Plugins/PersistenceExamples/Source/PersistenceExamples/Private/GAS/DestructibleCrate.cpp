// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAS/DestructibleCrate.h"
#include "GAS/PersistedAbilitySystemComponent.h"
#include "GAS/HealthAttributeSet.h"
#include "GeometryCollection/PersistedGeometryCollectionComponent.h"
#include "PersistenceExamples.h"

ADestructibleCrate::ADestructibleCrate()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Persisted GeomColComp remembers the transforms of fractured pieces
	GeometryCollection = CreateDefaultSubobject<UPersistedGeometryCollectionComponent>(TEXT("GeometryCollection"));
	SetRootComponent(GeometryCollection);

	// Persisted ASC remembers GameplayEffects with 'Persistable' tag and their remaining durations
	AbilitySystemComponent = CreateDefaultSubobject<UPersistedAbilitySystemComponent>(TEXT("PersistedAbilitySystemComp"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// HealthSet is directly targetable via LevelStreamingPersistence since attributes are just property values
	HealthSet = CreateDefaultSubobject<UHealthAttributeSet>(TEXT("HealthSet"));
}

UAbilitySystemComponent* ADestructibleCrate::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ADestructibleCrate::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}
