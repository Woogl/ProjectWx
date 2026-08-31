// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SubclassOf.h"
#include "WxSaveGame.h"
#include "WxSaveGameSubsystem.generated.h"

class UWorld;

namespace WxSave
{
	constexpr const TCHAR* DefaultSaveSlotName = TEXT("Default");
}

/** 인메모리 SaveGame 수명, 디스크 I/O, 저장 파일 트래블을 담당한다. */
UCLASS()
class WXSAVE_API UWxSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UWxSaveGame* GetSaveGame() const;
	bool IsTravelingFromSaveFile() const;

	UWxSaveGame* StartNewSaveFile(const FString& SlotName, int32 UserIndex, TSubclassOf<UWxSaveGame> SpecificClass);
	UWxSaveGame* LoadFromFile(const FString& SlotName = FString(), int32 UserIndex = 0, bool bStartTravel = true);
	void TravelFromSaveFile();

	bool SaveToFile(const FString& SlotName = FString(), int32 UserIndex = 0, const FTransform* ResumeTransform = nullptr);
	bool DoesSaveFileExist(const FString& SlotName, int32 UserIndex) const;
	bool DeleteSaveFile(const FString& SlotName, int32 UserIndex);

	void SetTravelData(FWxSaveTravelData InTravelData);
	void SetStreamingLevelDataForMap(FName MapKey, TArray<uint8> Data);
	void SetInstancedActorManagerDataForLevel(FName MapKey, FName LevelKey, FName ManagerKey, TArray<uint8> Data);
	void SetMassEntityDataForMap(FName MapKey, TArray<FWxMassEntityConfigGroupSnapshot> Snapshots);

	void ReportTravelFromSaveFileComplete(UWorld* World);

	static FName GetStableMapPackageName(const UWorld* World);

	void LogSaveState() const;
	bool IsSaveInProgress() const;

	FSimpleMulticastDelegate OnSaveCompleted;

private:
	void BeginAsyncSaveToDisk();
	void HandleSaveFlushComplete();
	void HandleAsyncSaveFinished(const FString& SlotName, int32 UserIndex, bool bSuccess);
	void FinishSaveInProgress();

	static int32 GetCurrentSaveFormatVersion();

	bool bSaveInProgress = false;

	UPROPERTY()
	TObjectPtr<UWxSaveGame> SaveGame;

	bool bTravelingFromSaveFile = false;
};
