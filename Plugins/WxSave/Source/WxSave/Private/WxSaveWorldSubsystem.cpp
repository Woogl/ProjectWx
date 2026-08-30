// Copyright Woogle. All Rights Reserved.

#include "WxSaveWorldSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AttributeSet.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameMapsSettings.h"
#include "InstancedActorsManager.h"
#include "LevelStreamingPersistenceManager.h"
#include "MassSimulationSubsystem.h"
#include "Serialization/CustomVersion.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/StructuredArchive.h"
#include "Streaming/LevelStreamingDelegates.h"
#include "UObject/ObjectVersion.h"
#include "WxMassPersistence.h"
#include "WxSaveGameSubsystem.h"
#include "WxSaveModule.h"
#include "WxSaveSettings.h"

bool UWxSaveWorldSubsystem::IsTransitionWorld(const UWorld* World)
{
	if (!World)
	{
		return false;
	}

	const FSoftObjectPath& TransitionMap = GetDefault<UGameMapsSettings>()->TransitionMap;
	return !TransitionMap.IsNull()
		&& UWxSaveGameSubsystem::GetStableMapPackageName(World) == TransitionMap.GetAssetPath().GetPackageName();
}

bool UWxSaveWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && !World->IsNetMode(NM_Client);
}

void UWxSaveWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	USubsystem* PersistenceDependency = Collection.InitializeDependency<ULevelStreamingPersistenceManager>();
	Super::Initialize(Collection);

	if (!PersistenceDependency || !GetWorld() || !GetWorld()->GetGameInstance())
	{
		UE_LOG(LogWxSave, Warning, TEXT("WxSaveWorldSubsystem 초기화 실패: LSP 또는 GameInstance 없음"));
		return;
	}

	LevelMakingVisibleHandle = FLevelStreamingDelegates::OnLevelBeginMakingVisible.AddUObject(
		this,
		&UWxSaveWorldSubsystem::HandleLevelBeginMakingVisible);
	LevelMakingInvisibleHandle = FLevelStreamingDelegates::OnLevelBeginMakingInvisible.AddUObject(
		this,
		&UWxSaveWorldSubsystem::FlushInstancedActorManagerDataForLevel);
	PreActorTickHandle = FWorldDelegates::OnWorldPreActorTick.AddUObject(
		this,
		&UWxSaveWorldSubsystem::HandleWorldPreActorTick);

	if (GetDefault<UWxSaveSettings>()->bAutoSaveWhenLeavingMap)
	{
		WorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(
			this,
			&UWxSaveWorldSubsystem::HandleWorldBeginTearDown);
	}

	const UWxSaveGame* SaveGame = GetActiveSaveGame();
	if (!SaveGame)
	{
		return;
	}

	const FName MapKey = UWxSaveGameSubsystem::GetStableMapPackageName(GetWorld());
	const FWxWorldPersistenceEntry* Entry = SaveGame->SavedStatePerMap.Find(MapKey);
	if (!Entry || Entry->StreamingLevelData.IsEmpty())
	{
		return;
	}

	ULevelStreamingPersistenceManager* PersistenceManager = GetWorld()->GetSubsystem<ULevelStreamingPersistenceManager>();
	if (PersistenceManager && PersistenceManager->InitializeFrom(Entry->StreamingLevelData))
	{
		UE_LOG(LogWxSave, Log, TEXT("LSP 초기화: '%s', %d바이트"), *MapKey.ToString(), Entry->StreamingLevelData.Num());
	}
	else
	{
		UE_LOG(LogWxSave, Warning, TEXT("LSP 초기화 실패: '%s'"), *MapKey.ToString());
	}
}

void UWxSaveWorldSubsystem::Deinitialize()
{
	FLevelStreamingDelegates::OnLevelBeginMakingVisible.Remove(LevelMakingVisibleHandle);
	FLevelStreamingDelegates::OnLevelBeginMakingInvisible.Remove(LevelMakingInvisibleHandle);
	FWorldDelegates::OnWorldPreActorTick.Remove(PreActorTickHandle);
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldBeginTearDownHandle);

	if (UMassSimulationSubsystem* SimulationSubsystem = GetWorld()
		? GetWorld()->GetSubsystem<UMassSimulationSubsystem>()
		: nullptr)
	{
		SimulationSubsystem->GetOnSimulationStarted().Remove(SimulationStartedHandle);
	}

	Super::Deinitialize();
}

void UWxSaveWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (IsTransitionWorld(&InWorld))
	{
		return;
	}

	UMassSimulationSubsystem* SimulationSubsystem = GetWorld()->GetSubsystem<UMassSimulationSubsystem>();
	if (SimulationSubsystem && SimulationSubsystem->IsSimulationStarted())
	{
		RestoreMassEntityData();
	}
	else if (SimulationSubsystem)
	{
		bPendingMassRestore = true;
		SimulationStartedHandle = SimulationSubsystem->GetOnSimulationStarted().AddUObject(
			this,
			&UWxSaveWorldSubsystem::HandleMassSimulationStarted);
	}

	if (InWorld.PersistentLevel)
	{
		for (AActor* Actor : InWorld.PersistentLevel->Actors)
		{
			if (AInstancedActorsManager* Manager = Cast<AInstancedActorsManager>(Actor))
			{
				ManagersPendingRestore.AddUnique(Manager);
			}
		}
	}

	UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	if (!GameSubsystem || !GameSubsystem->IsTravelingFromSaveFile())
	{
		return;
	}

	const UWxSaveGame* SaveGame = GameSubsystem->GetSaveGame();
	const UWxSaveSettings* Settings = GetDefault<UWxSaveSettings>();
	if (Settings->bRestoreControlRotation && SaveGame && SaveGame->TravelData.bHasControlRotation)
	{
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			PlayerController->SetControlRotation(SaveGame->TravelData.ControlRotation);
		}
	}

	GameSubsystem->ReportTravelFromSaveFileComplete(&InWorld);
}

void UWxSaveWorldSubsystem::RequestSaveFlush(
	FOnSaveFlushComplete::FDelegate OnComplete,
	const FTransform* ResumeTransform)
{
	if (OnComplete.IsBound())
	{
		OnSaveFlushBroadcast.Add(OnComplete);
	}
	if (bSaveFlushInProgress)
	{
		return;
	}
	bSaveFlushInProgress = true;

	UWorld* World = GetWorld();
	if (World && !World->bIsTearingDown)
	{
		FlushMapTravelData(ResumeTransform);
		FlushPlayerStats();
	}
	FlushLevelStreamingPersistenceData();

	UWxMassPersistence::SnapshotEntities(
		this,
		FWxOnMassPreSnapshot::CreateUObject(this, &UWxSaveWorldSubsystem::PerformPreSaveMassTasks),
		FWxOnMassSnapshotComplete::CreateUObject(this, &UWxSaveWorldSubsystem::HandleSaveFlushMassPartFinished));
}

UWxSaveGameSubsystem* UWxSaveWorldSubsystem::GetGameSubsystem() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
}

UWxSaveGame* UWxSaveWorldSubsystem::GetActiveSaveGame() const
{
	const UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	return GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
}

void UWxSaveWorldSubsystem::FlushMapTravelData(const FTransform* ResumeTransform)
{
	UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	UWxSaveGame* SaveGame = GetActiveSaveGame();
	if (!GameSubsystem || !SaveGame)
	{
		return;
	}

	const UWxSaveSettings* Settings = GetDefault<UWxSaveSettings>();
	FWxSaveTravelData TravelData = SaveGame->TravelData;
	TravelData.Map = FSoftObjectPath(UWxSaveGameSubsystem::GetStableMapPackageName(GetWorld()).ToString());

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (Settings->bRestorePawnTransform)
	{
		if (ResumeTransform)
		{
			TravelData.PawnTransform = *ResumeTransform;
			TravelData.bHasPawnTransform = true;
		}
		else if (const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr)
		{
			TravelData.PawnTransform = Pawn->GetActorTransform();
			TravelData.bHasPawnTransform = true;
		}
	}
	else
	{
		TravelData.bHasPawnTransform = false;
	}

	if (Settings->bRestoreControlRotation && PlayerController)
	{
		TravelData.ControlRotation = PlayerController->GetControlRotation();
		TravelData.bHasControlRotation = true;
	}
	else if (!Settings->bRestoreControlRotation)
	{
		TravelData.bHasControlRotation = false;
	}

	GameSubsystem->SetTravelData(MoveTemp(TravelData));
}

void UWxSaveWorldSubsystem::FlushPlayerStats()
{
	UWxSaveGame* SaveGame = GetActiveSaveGame();
	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!SaveGame || !Pawn)
	{
		return;
	}

	SaveGame->PlayerStats.Reset();
	CapturePlayerStats(Pawn, SaveGame->PlayerStats);
	SaveGame->bHasPlayerStats = !SaveGame->PlayerStats.IsEmpty();
}

void UWxSaveWorldSubsystem::FlushLevelStreamingPersistenceData()
{
	UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	ULevelStreamingPersistenceManager* PersistenceManager = GetWorld()
		? GetWorld()->GetSubsystem<ULevelStreamingPersistenceManager>()
		: nullptr;
	if (!GameSubsystem || !PersistenceManager)
	{
		return;
	}

	TArray<uint8> SerializedData;
	if (PersistenceManager->SerializeTo(SerializedData, true))
	{
		const FName MapKey = UWxSaveGameSubsystem::GetStableMapPackageName(GetWorld());
		GameSubsystem->SetStreamingLevelDataForMap(MapKey, MoveTemp(SerializedData));
	}
	else
	{
		UE_LOG(LogWxSave, Warning, TEXT("LSP 직렬화 실패: 기존 맵 데이터 유지"));
	}
}

TArray<ULevel*> UWxSaveWorldSubsystem::GetVisibleLevels() const
{
	TArray<ULevel*> VisibleLevels;
	UWorld* World = GetWorld();
	if (!World)
	{
		return VisibleLevels;
	}

	if (World->PersistentLevel)
	{
		VisibleLevels.Add(World->PersistentLevel);
	}
	for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		ULevel* LoadedLevel = StreamingLevel ? StreamingLevel->GetLoadedLevel() : nullptr;
		if (LoadedLevel && LoadedLevel->bIsVisible)
		{
			VisibleLevels.Add(LoadedLevel);
		}
	}
	return VisibleLevels;
}

void UWxSaveWorldSubsystem::FlushInstancedActorManagers()
{
	OnPreFlushInstancedActorsData.Broadcast();
	for (ULevel* Level : GetVisibleLevels())
	{
		ULevelStreaming* StreamingLevel = GetWorld()->GetLevelStreamingForPackageName(Level->GetPackage()->GetFName());
		FlushInstancedActorManagerDataForLevel(GetWorld(), StreamingLevel, Level);
	}
}

void UWxSaveWorldSubsystem::FlushInstancedActorManagerDataForLevel(
	UWorld* World,
	const ULevelStreaming* LevelStreaming,
	ULevel* Level)
{
	if (!Level || World != GetWorld())
	{
		return;
	}

	UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	if (!GameSubsystem || GameSubsystem->IsTravelingFromSaveFile())
	{
		return;
	}

	const FName MapKey = UWxSaveGameSubsystem::GetStableMapPackageName(GetWorld());
	const FName LevelKey = LevelStreaming
		? LevelStreaming->GetWorldAssetPackageFName()
		: MapKey;

	for (AActor* Actor : Level->Actors)
	{
		AInstancedActorsManager* Manager = Cast<AInstancedActorsManager>(Actor);
		if (!IsValid(Manager))
		{
			continue;
		}

		TArray<uint8> BodyData;
		FMemoryWriter BodyWriter(BodyData);
		BodyWriter.ArIsSaveGame = true;
		{
			FStructuredArchiveFromArchive StructuredArchive(BodyWriter);
			Manager->Serialize(StructuredArchive.GetSlot().EnterRecord());
		}

		if (BodyWriter.IsError() || BodyData.IsEmpty())
		{
			UE_LOG(LogWxSave, Warning, TEXT("IAM '%s' 직렬화 실패"), *Manager->GetName());
			continue;
		}

		FPackageFileVersion PackageVersion = GPackageFileUEVersion;
		FCustomVersionContainer CustomVersions = BodyWriter.GetCustomVersions();
		TArray<uint8> Data;
		FMemoryWriter Writer(Data);
		Writer.ArIsSaveGame = true;
		Writer << PackageVersion;
		CustomVersions.Serialize(Writer);
		Writer.Serialize(BodyData.GetData(), BodyData.Num());

		if (!Writer.IsError() && !Data.IsEmpty())
		{
			GameSubsystem->SetInstancedActorManagerDataForLevel(
				MapKey,
				LevelKey,
				Manager->GetFName(),
				MoveTemp(Data));
		}
	}
}

void UWxSaveWorldSubsystem::HandleLevelBeginMakingVisible(
	UWorld* World,
	const ULevelStreaming* LevelStreaming,
	ULevel* Level)
{
	if (World != GetWorld() || !Level)
	{
		return;
	}

	for (AActor* Actor : Level->Actors)
	{
		if (AInstancedActorsManager* Manager = Cast<AInstancedActorsManager>(Actor))
		{
			ManagersPendingRestore.AddUnique(Manager);
		}
	}
}

void UWxSaveWorldSubsystem::HandleWorldPreActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (World != GetWorld() || ManagersPendingRestore.IsEmpty())
	{
		return;
	}

	for (int32 Index = ManagersPendingRestore.Num() - 1; Index >= 0; --Index)
	{
		AInstancedActorsManager* Manager = ManagersPendingRestore[Index];
		if (!IsValid(Manager))
		{
			ManagersPendingRestore.RemoveAt(Index);
			continue;
		}
		if (!Manager->GetInstancedActorSubsystem())
		{
			continue;
		}
		if (Manager->HasSpawnedEntities())
		{
			UE_LOG(LogWxSave, Error, TEXT("IAM '%s'가 복원 전에 엔티티를 생성했다. IA.DeferSpawnEntities를 활성화해야 한다."), *Manager->GetName());
			ManagersPendingRestore.RemoveAt(Index);
			continue;
		}

		RestoreManager(Manager);
		ManagersPendingRestore.RemoveAt(Index);
	}
}

void UWxSaveWorldSubsystem::RestoreManager(AInstancedActorsManager* Manager)
{
	const UWxSaveGame* SaveGame = GetActiveSaveGame();
	if (!SaveGame || !Manager || !Manager->GetLevel())
	{
		return;
	}

	const FName MapKey = UWxSaveGameSubsystem::GetStableMapPackageName(GetWorld());
	ULevel* Level = Manager->GetLevel();
	ULevelStreaming* StreamingLevel = GetWorld()->GetLevelStreamingForPackageName(Level->GetPackage()->GetFName());
	const FName LevelKey = StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : MapKey;

	const FWxWorldPersistenceEntry* WorldEntry = SaveGame->SavedStatePerMap.Find(MapKey);
	const FWxStreamingLevelPersistenceEntry* LevelEntry = WorldEntry
		? WorldEntry->SavedStatePerStreamingLevel.Find(LevelKey)
		: nullptr;
	const FWxInstancedActorManagerState* State = LevelEntry
		? LevelEntry->InstancedActorManagerDeltas.Find(Manager->GetFName())
		: nullptr;
	if (!State || State->Data.IsEmpty())
	{
		return;
	}

	FMemoryReader Reader(State->Data);
	Reader.ArIsSaveGame = true;
	FPackageFileVersion PackageVersion;
	FCustomVersionContainer CustomVersions;
	Reader << PackageVersion;
	CustomVersions.Serialize(Reader);
	Reader.SetUEVer(PackageVersion);
	Reader.SetCustomVersions(CustomVersions);

	FStructuredArchiveFromArchive StructuredArchive(Reader);
	Manager->Serialize(StructuredArchive.GetSlot().EnterRecord());
	if (Reader.IsError())
	{
		UE_LOG(LogWxSave, Warning, TEXT("IAM '%s' 복원 중 직렬화 오류"), *Manager->GetName());
	}
}

void UWxSaveWorldSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
	if (World != GetWorld())
	{
		return;
	}

	const UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	if (!GameSubsystem || GameSubsystem->IsTravelingFromSaveFile())
	{
		return;
	}

	RequestSaveFlush(FOnSaveFlushComplete::FDelegate());
}

void UWxSaveWorldSubsystem::PerformPreSaveMassTasks()
{
	FlushInstancedActorManagers();
	OnPreFlushMassEntityData.Broadcast();
}

void UWxSaveWorldSubsystem::HandleSaveFlushMassPartFinished(
	TArray<FWxMassEntityConfigGroupSnapshot> Snapshots)
{
	if (UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem())
	{
		GameSubsystem->SetMassEntityDataForMap(
			UWxSaveGameSubsystem::GetStableMapPackageName(GetWorld()),
			MoveTemp(Snapshots));
	}

	FOnSaveFlushComplete LocalBroadcast = OnSaveFlushBroadcast;
	OnSaveFlushBroadcast.Clear();
	bSaveFlushInProgress = false;
	LocalBroadcast.Broadcast();
}

void UWxSaveWorldSubsystem::RestoreMassEntityData()
{
	const UWxSaveGame* SaveGame = GetActiveSaveGame();
	if (!SaveGame)
	{
		return;
	}

	const FName MapKey = UWxSaveGameSubsystem::GetStableMapPackageName(GetWorld());
	const FWxWorldPersistenceEntry* WorldEntry = SaveGame->SavedStatePerMap.Find(MapKey);
	if (!WorldEntry || WorldEntry->MassEntitySnapshots.IsEmpty())
	{
		return;
	}

	RestoringMassSnapshotCount = WorldEntry->MassEntitySnapshots.Num();
	RestoringMassMapKey = MapKey;
	UWxMassPersistence::RestoreEntities(
		this,
		WorldEntry->MassEntitySnapshots,
		FWxOnMassRestoreComplete::CreateUObject(this, &UWxSaveWorldSubsystem::HandleMassRestoreComplete));
}

void UWxSaveWorldSubsystem::HandleMassSimulationStarted(UWorld* World)
{
	if (World != GetWorld() || !bPendingMassRestore)
	{
		return;
	}

	bPendingMassRestore = false;
	if (UMassSimulationSubsystem* SimulationSubsystem = GetWorld()->GetSubsystem<UMassSimulationSubsystem>())
	{
		SimulationSubsystem->GetOnSimulationStarted().Remove(SimulationStartedHandle);
	}
	RestoreMassEntityData();
}

void UWxSaveWorldSubsystem::HandleMassRestoreComplete()
{
	UE_LOG(LogWxSave, Log, TEXT("Mass 스냅샷 복원 완료: %d개 그룹, 맵 '%s'"),
		RestoringMassSnapshotCount,
		*RestoringMassMapKey.ToString());
	RestoringMassSnapshotCount = 0;
	RestoringMassMapKey = NAME_None;
}

void UWxSaveWorldSubsystem::CapturePlayerStats(AActor* PlayerActor, TMap<FName, float>& OutStats)
{
	const UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(PlayerActor);
	if (!AbilitySystem)
	{
		return;
	}

	for (const UAttributeSet* Set : AbilitySystem->GetSpawnedAttributes())
	{
		if (!Set)
		{
			continue;
		}

		for (TFieldIterator<FStructProperty> It(Set->GetClass()); It; ++It)
		{
			if (It->Struct == FGameplayAttributeData::StaticStruct() && It->HasAnyPropertyFlags(CPF_Net))
			{
				const FGameplayAttribute Attribute(*It);
				OutStats.Add(It->GetFName(), AbilitySystem->GetNumericAttributeBase(Attribute));
			}
		}
	}
}

void UWxSaveWorldSubsystem::ApplyPlayerStats(AActor* PlayerActor, const TMap<FName, float>& InStats)
{
	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(PlayerActor);
	if (!AbilitySystem)
	{
		return;
	}

	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (const UAttributeSet* Set : AbilitySystem->GetSpawnedAttributes())
		{
			if (!Set)
			{
				continue;
			}

			for (TFieldIterator<FStructProperty> It(Set->GetClass()); It; ++It)
			{
				if (It->Struct != FGameplayAttributeData::StaticStruct())
				{
					continue;
				}

				const float* SavedValue = InStats.Find(It->GetFName());
				if (!SavedValue)
				{
					continue;
				}

				const FGameplayAttribute Attribute(*It);
				if (Pass == 1 && AbilitySystem->GetNumericAttributeBase(Attribute) == *SavedValue)
				{
					continue;
				}
				AbilitySystem->SetNumericAttributeBase(Attribute, *SavedValue);
			}
		}
	}
}
