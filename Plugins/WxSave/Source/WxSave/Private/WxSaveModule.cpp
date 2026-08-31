// Copyright Woogle. All Rights Reserved.

#include "WxSaveModule.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "LevelStreamingPersistenceModule.h"
#include "MassActorSubsystem.h"
#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"
#include "WxPersistableActorReferenceManager.h"
#include "WxSavable.h"

DEFINE_LOG_CATEGORY(LogWxSave);

namespace
{
	const FName RelativeLocationName(TEXT("RelativeLocation"));
	const FName RelativeRotationName(TEXT("RelativeRotation"));
}

void FWxSaveModule::StartupModule()
{
	ILevelStreamingPersistenceModule& PersistenceModule = ILevelStreamingPersistenceModule::Get();
	PersistenceModule.OnShouldPersistProperty(USceneComponent::StaticClass()).BindStatic(
		&FWxSaveModule::HandleShouldPersistSceneComponentProperty);
	PersistenceModule.OnPrePersistObject(UObject::StaticClass()).BindStatic(
		&FWxSaveModule::HandlePrePersistObject);
	PersistenceModule.OnPostRestoreObject(UObject::StaticClass()).BindStatic(
		&FWxSaveModule::HandlePostRestoreObject);
	PersistenceModule.OnPostRestoreObject(USceneComponent::StaticClass()).BindStatic(
		&FWxSaveModule::HandlePostRestoreSceneComponent);
	PersistenceModule.OnShouldPersistRuntimeActor(AActor::StaticClass()).BindStatic(
		&FWxSaveModule::HandleShouldPersistRuntimeActor);
	PersistenceModule.PostRestoreLevel.BindStatic(&FWxSaveModule::HandlePostRestoreLevel);
}

void FWxSaveModule::ShutdownModule()
{
	if (!ILevelStreamingPersistenceModule::IsAvailable()
		|| UObject::StaticClass()->HasAnyFlags(RF_BeginDestroyed))
	{
		return;
	}

	ILevelStreamingPersistenceModule& PersistenceModule = ILevelStreamingPersistenceModule::Get();
	PersistenceModule.OnShouldPersistProperty(USceneComponent::StaticClass()).Unbind();
	PersistenceModule.OnPrePersistObject(UObject::StaticClass()).Unbind();
	PersistenceModule.OnPostRestoreObject(UObject::StaticClass()).Unbind();
	PersistenceModule.OnPostRestoreObject(USceneComponent::StaticClass()).Unbind();
	PersistenceModule.OnShouldPersistRuntimeActor(AActor::StaticClass()).Unbind();
	PersistenceModule.PostRestoreLevel.Unbind();
}

void FWxSaveModule::HandlePrePersistObject(const UObject* Object)
{
	if (!Object)
	{
		return;
	}

	UObject* MutableObject = const_cast<UObject*>(Object);
	if (IWxSavable* Savable = Cast<IWxSavable>(MutableObject))
	{
		Savable->OnSavePreparing();
		return;
	}

	const UActorComponent* Component = Cast<UActorComponent>(Object);
	if (IWxSavable* OwnerSavable = Component ? Cast<IWxSavable>(Component->GetOwner()) : nullptr)
	{
		OwnerSavable->OnSavePreparing();
	}
}

bool FWxSaveModule::HandleShouldPersistSceneComponentProperty(
	const UObject* Object,
	const FProperty* Property)
{
	const USceneComponent* SceneComponent = Cast<USceneComponent>(Object);
	const AActor* Owner = SceneComponent ? SceneComponent->GetOwner() : nullptr;
	if (!SceneComponent || !Owner || !Property)
	{
		return false;
	}

	const UWorld* World = Owner->GetWorld();
	UMassActorSubsystem* MassActorSubsystem = World
		? World->GetSubsystem<UMassActorSubsystem>()
		: nullptr;
	if (MassActorSubsystem && MassActorSubsystem->GetEntityHandleFromActor(Owner).IsValid())
	{
		return false;
	}

	const FName PropertyName = Property->GetFName();
	if (PropertyName == RelativeLocationName || PropertyName == RelativeRotationName)
	{
		return Owner->IsNetStartupActor()
			&& Owner->GetRootComponent() == SceneComponent
			&& SceneComponent->Mobility == EComponentMobility::Movable;
	}

	return true;
}

void FWxSaveModule::HandlePostRestoreObject(
	const UObject* Object,
	const TArray<const FProperty*>& RestoredProperties)
{
	if (!Object || RestoredProperties.IsEmpty())
	{
		return;
	}

	UObject* MutableObject = const_cast<UObject*>(Object);
	IWxSavable* Savable = Cast<IWxSavable>(MutableObject);
	if (!Savable)
	{
		const UActorComponent* Component = Cast<UActorComponent>(Object);
		Savable = Component ? Cast<IWxSavable>(Component->GetOwner()) : nullptr;
	}

	if (Savable)
	{
		TArray<FName> RestoredPropertyNames;
		RestoredPropertyNames.Reserve(RestoredProperties.Num());
		for (const FProperty* Property : RestoredProperties)
		{
			if (Property)
			{
				RestoredPropertyNames.Add(Property->GetFName());
			}
		}
		Savable->OnSaveRestored(RestoredPropertyNames);
	}
}

void FWxSaveModule::HandlePostRestoreLevel(const ULevel* Level)
{
	UWorld* World = Level ? Level->GetWorld() : nullptr;
	if (UWxPersistableActorReferenceManager* Manager = World
		? World->GetSubsystem<UWxPersistableActorReferenceManager>()
		: nullptr)
	{
		Manager->OnLevelPostRestore(Level);
	}

	if (!Level)
	{
		return;
	}

	TArray<TWeakObjectPtr<AActor>> SavableActors;
	for (AActor* Actor : Level->Actors)
	{
		if (IsValid(Actor) && Cast<IWxSavable>(Actor))
		{
			SavableActors.Add(Actor);
		}
	}

	for (const TWeakObjectPtr<AActor>& Actor : SavableActors)
	{
		if (IWxSavable* Savable = Cast<IWxSavable>(Actor.Get()))
		{
			Savable->OnPostRestoreLevel();
		}
	}
}

void FWxSaveModule::HandlePostRestoreSceneComponent(
	const UObject* Object,
	const TArray<const FProperty*>& RestoredProperties)
{
	USceneComponent* SceneComponent = const_cast<USceneComponent*>(Cast<USceneComponent>(Object));
	if (!SceneComponent)
	{
		return;
	}

	for (const FProperty* Property : RestoredProperties)
	{
		if (Property
			&& (Property->GetFName() == RelativeLocationName
				|| Property->GetFName() == RelativeRotationName))
		{
			SceneComponent->UpdateComponentToWorld();
			return;
		}
	}
}

bool FWxSaveModule::HandleShouldPersistRuntimeActor(const AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}
	if (const IWxSavable* Savable = Cast<IWxSavable>(Actor);
		Savable && !Savable->ShouldPersistRuntimeActor())
	{
		return false;
	}

	const UWorld* World = Actor->GetWorld();
	UMassActorSubsystem* MassActorSubsystem = World
		? World->GetSubsystem<UMassActorSubsystem>()
		: nullptr;
	return !MassActorSubsystem || !MassActorSubsystem->GetEntityHandleFromActor(Actor).IsValid();
}

IMPLEMENT_MODULE(FWxSaveModule, WxSave)
