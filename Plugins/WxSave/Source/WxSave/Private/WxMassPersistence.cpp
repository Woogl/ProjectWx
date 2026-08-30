// Copyright Woogle. All Rights Reserved.

#include "WxMassPersistence.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "MassEntityQuery.h"
#include "MassEntityTemplate.h"
#include "MassExecutionContext.h"
#include "MassRequirements.h"
#include "MassSimulationSubsystem.h"
#include "MassSpawnerSubsystem.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "WxPersistableMassTrait.h"
#include "WxSaveModule.h"
#include "WxSaveSettings.h"

void UWxMassPersistence::SnapshotEntities(
	const UObject* WorldContextObject,
	FWxOnMassPreSnapshot PreSnapshot,
	FWxOnMassSnapshotComplete OnComplete)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		OnComplete.ExecuteIfBound({});
		return;
	}

	if (World->bIsTearingDown)
	{
		PreSnapshot.ExecuteIfBound();
		TArray<FWxMassEntityConfigGroupSnapshot> Snapshots;
		DoSnapshotWork(*World, Snapshots);
		OnComplete.ExecuteIfBound(MoveTemp(Snapshots));
		return;
	}

	UMassSimulationSubsystem* SimulationSubsystem = World->GetSubsystem<UMassSimulationSubsystem>();
	if (!SimulationSubsystem)
	{
		PreSnapshot.ExecuteIfBound();
		OnComplete.ExecuteIfBound({});
		return;
	}

	struct FSnapshotState
	{
		TWeakObjectPtr<UWorld> WeakWorld;
		TWeakObjectPtr<UMassSimulationSubsystem> WeakSimulationSubsystem;
		FWxOnMassPreSnapshot PreSnapshot;
		FWxOnMassSnapshotComplete OnComplete;
		FDelegateHandle PhaseHandle;
		FDelegateHandle TeardownHandle;
		bool bFired = false;

		void Fire()
		{
			if (bFired)
			{
				return;
			}
			bFired = true;

			UWorld* PinnedWorld = WeakWorld.Get();
			if (PinnedWorld)
			{
				PreSnapshot.ExecuteIfBound();
				TArray<FWxMassEntityConfigGroupSnapshot> Snapshots;
				UWxMassPersistence::DoSnapshotWork(*PinnedWorld, Snapshots);
				OnComplete.ExecuteIfBound(MoveTemp(Snapshots));
			}
			else
			{
				OnComplete.ExecuteIfBound({});
			}

			FWorldDelegates::OnWorldBeginTearDown.Remove(TeardownHandle);
			if (UMassSimulationSubsystem* PinnedSimulation = WeakSimulationSubsystem.Get())
			{
				PinnedSimulation->GetOnProcessingPhaseFinished(EMassProcessingPhase::FrameEnd).Remove(PhaseHandle);
			}
		}
	};

	TSharedRef<FSnapshotState> State = MakeShared<FSnapshotState>();
	State->WeakWorld = World;
	State->WeakSimulationSubsystem = SimulationSubsystem;
	State->PreSnapshot = MoveTemp(PreSnapshot);
	State->OnComplete = MoveTemp(OnComplete);

	// 두 전역 delegate가 같은 공유 상태를 소유해야 하므로 캡처 람다가 필요하다.
	State->PhaseHandle = SimulationSubsystem->GetOnProcessingPhaseFinished(EMassProcessingPhase::FrameEnd).AddLambda(
		[State](float DeltaSeconds)
		{
			State->Fire();
		});
	State->TeardownHandle = FWorldDelegates::OnWorldBeginTearDown.AddLambda(
		[State](UWorld* TornDownWorld)
		{
			if (TornDownWorld == State->WeakWorld.Get())
			{
				State->Fire();
			}
		});
}

void UWxMassPersistence::RestoreEntities(
	const UObject* WorldContextObject,
	TArray<FWxMassEntityConfigGroupSnapshot> Snapshots,
	FWxOnMassRestoreComplete OnComplete)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World || World->bIsTearingDown || Snapshots.IsEmpty())
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	UMassSimulationSubsystem* SimulationSubsystem = World->GetSubsystem<UMassSimulationSubsystem>();
	if (!SimulationSubsystem)
	{
		UE_LOG(LogWxSave, Warning, TEXT("Mass 복원 스킵: UMassSimulationSubsystem 없음"));
		OnComplete.ExecuteIfBound();
		return;
	}

	struct FRestoreState
	{
		TWeakObjectPtr<UWorld> WeakWorld;
		TWeakObjectPtr<UMassSimulationSubsystem> WeakSimulationSubsystem;
		TArray<FWxMassEntityConfigGroupSnapshot> Snapshots;
		FWxOnMassRestoreComplete OnComplete;
		FDelegateHandle PhaseHandle;
		FDelegateHandle TeardownHandle;
		bool bFired = false;

		void Fire(bool bDoWork)
		{
			if (bFired)
			{
				return;
			}
			bFired = true;

			UWorld* PinnedWorld = WeakWorld.Get();
			if (bDoWork && PinnedWorld)
			{
				UWxMassPersistence::DoRestoreWork(*PinnedWorld, Snapshots);
			}
			OnComplete.ExecuteIfBound();

			if (UMassSimulationSubsystem* PinnedSimulation = WeakSimulationSubsystem.Get())
			{
				PinnedSimulation->GetOnProcessingPhaseFinished(EMassProcessingPhase::FrameEnd).Remove(PhaseHandle);
			}
			FWorldDelegates::OnWorldBeginTearDown.Remove(TeardownHandle);
		}
	};

	TSharedRef<FRestoreState> State = MakeShared<FRestoreState>();
	State->WeakWorld = World;
	State->WeakSimulationSubsystem = SimulationSubsystem;
	State->Snapshots = MoveTemp(Snapshots);
	State->OnComplete = MoveTemp(OnComplete);

	// FrameEnd와 teardown 중 먼저 온 한 경로만 실행해야 하므로 공유 상태 캡처가 필요하다.
	State->PhaseHandle = SimulationSubsystem->GetOnProcessingPhaseFinished(EMassProcessingPhase::FrameEnd).AddLambda(
		[State](float DeltaSeconds)
		{
			State->Fire(true);
		});
	State->TeardownHandle = FWorldDelegates::OnWorldBeginTearDown.AddLambda(
		[State](UWorld* TornDownWorld)
		{
			if (TornDownWorld == State->WeakWorld.Get())
			{
				State->Fire(false);
			}
		});
}

void UWxMassPersistence::DoSnapshotWork(UWorld& World, TArray<FWxMassEntityConfigGroupSnapshot>& OutSnapshots)
{
	UMassSpawnerSubsystem* SpawnerSubsystem = World.GetSubsystem<UMassSpawnerSubsystem>();
	if (!SpawnerSubsystem)
	{
		UE_LOG(LogWxSave, Warning, TEXT("Mass 스냅샷 스킵: UMassSpawnerSubsystem 없음"));
		return;
	}

	FMassEntityManager& EntityManager = SpawnerSubsystem->GetEntityManagerChecked();
	const UWxSaveSettings* Settings = GetDefault<UWxSaveSettings>();

	TSet<const UScriptStruct*> FragmentAllowList;
	for (const FName& PathName : Settings->MassFragmentsToSerialize)
	{
		if (const UScriptStruct* FragmentType = FindObject<UScriptStruct>(nullptr, *PathName.ToString()))
		{
			FragmentAllowList.Add(FragmentType);
		}
		else
		{
			UE_LOG(LogWxSave, Warning, TEXT("Mass fragment '%s'를 찾지 못해 저장에서 제외"), *PathName.ToString());
		}
	}

	if (FragmentAllowList.IsEmpty())
	{
		return;
	}

	TMap<UMassEntityConfigAsset*, TArray<FMassEntityHandle>> EntitiesByConfig;
	FMassEntityQuery DiscoveryQuery;
	DiscoveryQuery.Initialize(EntityManager.AsShared());
	DiscoveryQuery.AddRequirement<FWxPersistableEntityConfigFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);

	FMassExecutionContext DiscoveryContext(EntityManager);
	// Mass query가 chunk callback만 제공하므로 람다가 필요하다.
	DiscoveryQuery.ForEachEntityChunk(DiscoveryContext, [&EntityManager, &EntitiesByConfig](FMassExecutionContext& Context)
	{
		for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
		{
			const FMassEntityHandle Entity = Context.GetEntity(Index);
			const FWxPersistableEntityConfigFragment& Origin =
				EntityManager.GetFragmentDataChecked<FWxPersistableEntityConfigFragment>(Entity);
			if (UMassEntityConfigAsset* Config = Origin.EntityConfig)
			{
				EntitiesByConfig.FindOrAdd(Config).Add(Entity);
			}
		}
	});

	const UScriptStruct* OriginFragmentType = FWxPersistableEntityConfigFragment::StaticStruct();
	for (const TPair<UMassEntityConfigAsset*, TArray<FMassEntityHandle>>& Pair : EntitiesByConfig)
	{
		UMassEntityConfigAsset* ConfigAsset = Pair.Key;
		const TArray<FMassEntityHandle>& Entities = Pair.Value;
		if (Entities.IsEmpty())
		{
			continue;
		}

		const FMassEntityTemplate& EntityTemplate = ConfigAsset->GetOrCreateEntityTemplate(World);
		const FMassArchetypeHandle& Archetype = EntityTemplate.GetArchetype();
		if (!Archetype.IsValid())
		{
			continue;
		}

		TArray<const UScriptStruct*> FragmentTypes;
		// 엔진 archetype 순회 API가 callback만 제공하므로 람다가 필요하다.
		FMassEntityManager::ForEachArchetypeFragmentType(
			Archetype,
			[&FragmentTypes, &FragmentAllowList, OriginFragmentType](const UScriptStruct* FragmentType)
			{
				if (FragmentType != OriginFragmentType && FragmentAllowList.Contains(FragmentType))
				{
					FragmentTypes.Add(FragmentType);
				}
			});

		if (FragmentTypes.IsEmpty())
		{
			continue;
		}

		TArray<uint8> Data;
		FMemoryWriter Writer(Data);
		for (const FMassEntityHandle& Entity : Entities)
		{
			for (const UScriptStruct* FragmentType : FragmentTypes)
			{
				FStructView FragmentView = EntityManager.GetFragmentDataStruct(Entity, FragmentType);
				Writer.Serialize(FragmentView.GetMemory(), FragmentType->GetStructureSize());
			}
		}

		FWxMassEntityConfigGroupSnapshot& Snapshot = OutSnapshots.AddDefaulted_GetRef();
		for (const UScriptStruct* FragmentType : FragmentTypes)
		{
			FWxMassFragmentLayout& Layout = Snapshot.FragmentLayout.AddDefaulted_GetRef();
			Layout.Type = FSoftObjectPath(FragmentType);
			Layout.SizeInBytes = FragmentType->GetStructureSize();
		}
		Snapshot.EntityCount = Entities.Num();
		Snapshot.Data = MoveTemp(Data);
		Snapshot.SourceConfigAsset = FSoftObjectPath(ConfigAsset);
	}
}

void UWxMassPersistence::DoRestoreWork(UWorld& World, TConstArrayView<FWxMassEntityConfigGroupSnapshot> Snapshots)
{
	UMassSpawnerSubsystem* SpawnerSubsystem = World.GetSubsystem<UMassSpawnerSubsystem>();
	if (!SpawnerSubsystem)
	{
		UE_LOG(LogWxSave, Warning, TEXT("Mass 복원 스킵: UMassSpawnerSubsystem 없음"));
		return;
	}

	FMassEntityManager& EntityManager = SpawnerSubsystem->GetEntityManagerChecked();
	const UWxSaveSettings* Settings = GetDefault<UWxSaveSettings>();

	TSet<const UScriptStruct*> FragmentAllowList;
	for (const FName& PathName : Settings->MassFragmentsToSerialize)
	{
		if (const UScriptStruct* FragmentType = FindObject<UScriptStruct>(nullptr, *PathName.ToString()))
		{
			FragmentAllowList.Add(FragmentType);
		}
	}

	if (FragmentAllowList.IsEmpty())
	{
		return;
	}

	for (const FWxMassEntityConfigGroupSnapshot& Snapshot : Snapshots)
	{
		if (Snapshot.EntityCount <= 0 || Snapshot.Data.IsEmpty())
		{
			continue;
		}

		UMassEntityConfigAsset* RestoredConfig = Cast<UMassEntityConfigAsset>(Snapshot.SourceConfigAsset.TryLoad());
		if (!RestoredConfig)
		{
			UE_LOG(LogWxSave, Warning, TEXT("Mass EntityConfig '%s'를 로드하지 못해 스냅샷 복원 스킵"), *Snapshot.SourceConfigAsset.ToString());
			continue;
		}

		struct FResolvedFragment
		{
			const UScriptStruct* Struct = nullptr;
			int32 SavedSize = 0;
		};

		TArray<FResolvedFragment> ResolvedFragments;
		int32 ActiveFragmentCount = 0;
		for (const FWxMassFragmentLayout& Layout : Snapshot.FragmentLayout)
		{
			FResolvedFragment& Resolved = ResolvedFragments.AddDefaulted_GetRef();
			Resolved.SavedSize = Layout.SizeInBytes;
			Resolved.Struct = Cast<UScriptStruct>(Layout.Type.TryLoad());

			if (!Resolved.Struct)
			{
				UE_LOG(LogWxSave, Warning, TEXT("Mass fragment '%s'를 로드하지 못해 엔티티당 %d바이트 스킵"), *Layout.Type.ToString(), Layout.SizeInBytes);
			}
			else if (Resolved.Struct->GetStructureSize() != Layout.SizeInBytes)
			{
				UE_LOG(LogWxSave, Warning, TEXT("Mass fragment '%s' 크기 변경(%d -> %d)으로 저장 값 스킵"),
					*Layout.Type.GetAssetName(), Layout.SizeInBytes, Resolved.Struct->GetStructureSize());
				Resolved.Struct = nullptr;
			}
			else if (!FragmentAllowList.Contains(Resolved.Struct))
			{
				Resolved.Struct = nullptr;
			}
			else
			{
				++ActiveFragmentCount;
			}
		}

		if (ActiveFragmentCount == 0)
		{
			continue;
		}

		TArray<FMassEntityHandle> NewEntities;
		{
			const FMassEntityTemplate& EntityTemplate = RestoredConfig->GetOrCreateEntityTemplate(World);
			TSharedPtr<FMassEntityManager::FEntityCreationContext> CreationContext =
				SpawnerSubsystem->SpawnEntities(EntityTemplate, static_cast<uint32>(Snapshot.EntityCount), NewEntities);

			for (FMassEntityHandle Entity : NewEntities)
			{
				if (FWxPersistableEntityConfigFragment* Origin = EntityManager.GetFragmentDataPtr<FWxPersistableEntityConfigFragment>(Entity))
				{
					Origin->EntityConfig = RestoredConfig;
				}
			}

			FMemoryReader Reader(Snapshot.Data);
			for (const FMassEntityHandle& Entity : NewEntities)
			{
				for (const FResolvedFragment& Resolved : ResolvedFragments)
				{
					if (Resolved.Struct)
					{
						FStructView FragmentView = EntityManager.GetFragmentDataStruct(Entity, Resolved.Struct);
						Reader.Serialize(FragmentView.GetMemory(), Resolved.SavedSize);
					}
					else
					{
						Reader.Seek(Reader.Tell() + Resolved.SavedSize);
					}
				}
			}
		}

		UE_LOG(LogWxSave, Log, TEXT("Mass 복원: %d개 엔티티 (%s)"), NewEntities.Num(), *Snapshot.SourceConfigAsset.GetAssetName());
	}
}
