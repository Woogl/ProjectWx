// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "WxSaveGame.h"
#include "WxSaveWorldSubsystem.generated.h"

class AInstancedActorsManager;
struct FActorsInitializedParams;
class ULevel;
class ULevelStreaming;
class UWxSaveGame;
class UWxSaveGameSubsystem;

/** LSP, Instanced Actors, Mass와 엔진 PlayerStart 재개 지점의 월드 단위 저장·복원을 조율한다. */
UCLASS()
class WXSAVE_API UWxSaveWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE(FPreFlushInstancedActorsData);
	DECLARE_MULTICAST_DELEGATE(FPreFlushMassEntityData);
	DECLARE_MULTICAST_DELEGATE(FOnSaveFlushComplete);

	FPreFlushInstancedActorsData OnPreFlushInstancedActorsData;
	FPreFlushMassEntityData OnPreFlushMassEntityData;

	void RequestSaveFlush(FOnSaveFlushComplete::FDelegate OnComplete, const FTransform* ResumeTransform = nullptr);

	static bool IsTransitionWorld(const UWorld* World);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UWxSaveGameSubsystem* GetGameSubsystem() const;
	UWxSaveGame* GetActiveSaveGame() const;

	void FlushMapTravelData(const FTransform* ResumeTransform);
	void FlushLevelStreamingPersistenceData();
	void FlushInstancedActorManagers();
	void FlushInstancedActorManagerDataForLevel(UWorld* World, const ULevelStreaming* LevelStreaming, ULevel* Level);
	TArray<ULevel*> GetVisibleLevels() const;

	void HandleLevelBeginMakingVisible(UWorld* World, const ULevelStreaming* LevelStreaming, ULevel* Level);
	void HandleWorldInitializedActors(const FActorsInitializedParams& Params);
	void HandleWorldPreActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
	void HandleWorldBeginTearDown(UWorld* World);
	void RestoreManager(AInstancedActorsManager* Manager);

	void PerformPreSaveMassTasks();
	void HandleSaveFlushMassPartFinished(TArray<FWxMassEntityConfigGroupSnapshot> Snapshots);
	void RestoreMassEntityData();
	void HandleMassSimulationStarted(UWorld* World);
	void HandleMassRestoreComplete();

	FDelegateHandle LevelMakingVisibleHandle;
	FDelegateHandle LevelMakingInvisibleHandle;
	FDelegateHandle WorldInitializedActorsHandle;
	FDelegateHandle PreActorTickHandle;
	FDelegateHandle WorldBeginTearDownHandle;
	FDelegateHandle SimulationStartedHandle;

	bool bPendingMassRestore = false;
	bool bSaveFlushInProgress = false;
	bool bHasPendingPlayerStart = false;
	FTransform PendingPlayerStartTransform = FTransform::Identity;

	FOnSaveFlushComplete OnSaveFlushBroadcast;

	UPROPERTY()
	TArray<TObjectPtr<AInstancedActorsManager>> ManagersPendingRestore;

	int32 RestoringMassSnapshotCount = 0;
	FName RestoringMassMapKey;
};
