// Copyright Woogle. All Rights Reserved.

#include "WxSaveGameSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "WxSaveModule.h"
#include "WxSaveWorldSubsystem.h"

namespace
{
	void HandleDumpSaveCommand(UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		if (UWxSaveGameSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr)
		{
			Subsystem->LogSaveState();
		}
		else
		{
			UE_LOG(LogWxSave, Warning, TEXT("Wx.Save.Dump: WxSaveGameSubsystem을 찾을 수 없음"));
		}
	}

	FAutoConsoleCommandWithWorld GWxSaveDumpCommand(
		TEXT("Wx.Save.Dump"),
		TEXT("현재 WxSave 메모리 슬롯의 맵별 LSP/IAM/Mass 상태를 로그로 덤프한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&HandleDumpSaveCommand));
}

void UWxSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartNewSaveFile(WxSave::DefaultSaveSlotName, 0, UWxSaveGame::StaticClass());
}

UWxSaveGame* UWxSaveGameSubsystem::GetSaveGame() const
{
	return SaveGame;
}

bool UWxSaveGameSubsystem::IsTravelingFromSaveFile() const
{
	return bTravelingFromSaveFile;
}

UWxSaveGame* UWxSaveGameSubsystem::StartNewSaveFile(
	const FString& SlotName,
	int32 UserIndex,
	TSubclassOf<UWxSaveGame> SpecificClass)
{
	if (!SpecificClass)
	{
		UE_LOG(LogWxSave, Warning, TEXT("StartNewSaveFile: UWxSaveGame 서브클래스가 필요하다."));
		return nullptr;
	}

	UWxSaveGame* NewSaveGame = Cast<UWxSaveGame>(UGameplayStatics::CreateSaveGameObject(SpecificClass));
	if (!NewSaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("StartNewSaveFile: '%s' 생성 실패"), *SpecificClass->GetName());
		return nullptr;
	}

	NewSaveGame->SaveFormatVersion = GetCurrentSaveFormatVersion();
	NewSaveGame->SlotName = SlotName.IsEmpty() ? WxSave::DefaultSaveSlotName : SlotName;
	NewSaveGame->UserIndex = UserIndex;
	SaveGame = NewSaveGame;
	bTravelingFromSaveFile = false;

	UE_LOG(LogWxSave, Log, TEXT("새 저장 시작: 슬롯 '%s', 포맷 %d"), *SaveGame->SlotName, SaveGame->SaveFormatVersion);
	return SaveGame;
}

UWxSaveGame* UWxSaveGameSubsystem::LoadFromFile(const FString& SlotName, int32 UserIndex, bool bStartTravel)
{
	FString TargetSlot = SlotName;
	int32 TargetUserIndex = UserIndex;
	if (TargetSlot.IsEmpty() && SaveGame)
	{
		TargetSlot = SaveGame->SlotName;
		TargetUserIndex = SaveGame->UserIndex;
	}
	if (TargetSlot.IsEmpty())
	{
		TargetSlot = WxSave::DefaultSaveSlotName;
	}

	UWxSaveGame* Loaded = Cast<UWxSaveGame>(UGameplayStatics::LoadGameFromSlot(TargetSlot, TargetUserIndex));
	if (Loaded && Loaded->SaveFormatVersion == GetCurrentSaveFormatVersion())
	{
		SaveGame = Loaded;
		SaveGame->SlotName = TargetSlot;
		SaveGame->UserIndex = TargetUserIndex;
		UE_LOG(LogWxSave, Log, TEXT("저장 로드: 슬롯 '%s', 맵 상태 %d개"), *TargetSlot, SaveGame->SavedStatePerMap.Num());
	}
	else
	{
		if (Loaded)
		{
			UE_LOG(LogWxSave, Warning, TEXT("슬롯 '%s' 포맷 %d는 현재 포맷 %d와 호환되지 않아 새 저장으로 초기화한다."),
				*TargetSlot, Loaded->SaveFormatVersion, GetCurrentSaveFormatVersion());
		}
		else
		{
			UE_LOG(LogWxSave, Log, TEXT("슬롯 '%s' 파일 없음 또는 손상: 새 저장으로 시작"), *TargetSlot);
		}
		StartNewSaveFile(TargetSlot, TargetUserIndex, UWxSaveGame::StaticClass());
	}

	if (bStartTravel)
	{
		TravelFromSaveFile();
	}

	return SaveGame;
}

void UWxSaveGameSubsystem::TravelFromSaveFile()
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("TravelFromSaveFile: 활성 SaveGame 없음"));
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogWxSave, Warning, TEXT("TravelFromSaveFile: 월드 없음"));
		return;
	}

	const FSoftObjectPath& Map = SaveGame->TravelData.Map;
	const FString TravelMap = Map.IsNull()
		? GetStableMapPackageName(World).ToString()
		: Map.GetAssetPath().GetPackageName().ToString();

	bTravelingFromSaveFile = true;
	UE_LOG(LogWxSave, Log, TEXT("저장 파일 트래블: '%s'"), *TravelMap);
	if (!World->ServerTravel(TravelMap, true))
	{
		bTravelingFromSaveFile = false;
		UE_LOG(LogWxSave, Warning, TEXT("ServerTravel 시작 실패: '%s'"), *TravelMap);
	}
}

bool UWxSaveGameSubsystem::SaveToFile(
	const FString& SlotName,
	int32 UserIndex,
	const FTransform* ResumeTransform)
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("SaveToFile: 활성 SaveGame 없음"));
		return false;
	}
	if (bSaveInProgress)
	{
		UE_LOG(LogWxSave, Warning, TEXT("SaveToFile: 이전 저장이 진행 중이라 요청 거절"));
		return false;
	}

	if (!SlotName.IsEmpty())
	{
		SaveGame->SlotName = SlotName;
		SaveGame->UserIndex = UserIndex;
	}

	bSaveInProgress = true;
	UWxSaveWorldSubsystem* WorldSubsystem = nullptr;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWorld* World = GameInstance->GetWorld())
		{
			WorldSubsystem = World->GetSubsystem<UWxSaveWorldSubsystem>();
		}
	}

	if (WorldSubsystem)
	{
		WorldSubsystem->RequestSaveFlush(
			UWxSaveWorldSubsystem::FOnSaveFlushComplete::FDelegate::CreateUObject(
				this,
				&UWxSaveGameSubsystem::HandleSaveFlushComplete),
			ResumeTransform);
	}
	else
	{
		HandleSaveFlushComplete();
	}

	return true;
}

bool UWxSaveGameSubsystem::DoesSaveFileExist(const FString& SlotName, int32 UserIndex) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

bool UWxSaveGameSubsystem::DeleteSaveFile(const FString& SlotName, int32 UserIndex)
{
	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
	if (bDeleted)
	{
		UE_LOG(LogWxSave, Log, TEXT("슬롯 '%s' 삭제 성공"), *SlotName);
	}
	else
	{
		UE_LOG(LogWxSave, Warning, TEXT("슬롯 '%s' 삭제 실패"), *SlotName);
	}
	return bDeleted;
}

void UWxSaveGameSubsystem::SetTravelData(FWxSaveTravelData InTravelData)
{
	if (SaveGame)
	{
		SaveGame->TravelData = MoveTemp(InTravelData);
	}
}

void UWxSaveGameSubsystem::SetStreamingLevelDataForMap(FName MapKey, TArray<uint8> Data)
{
	if (SaveGame)
	{
		SaveGame->SavedStatePerMap.FindOrAdd(MapKey).StreamingLevelData = MoveTemp(Data);
	}
}

void UWxSaveGameSubsystem::SetInstancedActorManagerDataForLevel(
	FName MapKey,
	FName LevelKey,
	FName ManagerKey,
	TArray<uint8> Data)
{
	if (!SaveGame)
	{
		return;
	}

	FWxWorldPersistenceEntry& WorldEntry = SaveGame->SavedStatePerMap.FindOrAdd(MapKey);
	FWxStreamingLevelPersistenceEntry& LevelEntry = WorldEntry.SavedStatePerStreamingLevel.FindOrAdd(LevelKey);
	LevelEntry.InstancedActorManagerDeltas.FindOrAdd(ManagerKey).Data = MoveTemp(Data);
}

void UWxSaveGameSubsystem::SetMassEntityDataForMap(
	FName MapKey,
	TArray<FWxMassEntityConfigGroupSnapshot> Snapshots)
{
	if (SaveGame)
	{
		SaveGame->SavedStatePerMap.FindOrAdd(MapKey).MassEntitySnapshots = MoveTemp(Snapshots);
	}
}

bool UWxSaveGameSubsystem::TryGetPlayerTransform(const UWorld* World, FTransform& OutTransform) const
{
	if (!SaveGame || !World || !SaveGame->TravelData.bHasPawnTransform)
	{
		return false;
	}

	const FName SavedMap = SaveGame->TravelData.Map.IsNull()
		? NAME_None
		: SaveGame->TravelData.Map.GetAssetPath().GetPackageName();
	if (SavedMap != GetStableMapPackageName(World))
	{
		return false;
	}

	OutTransform = SaveGame->TravelData.PawnTransform;
	return true;
}

void UWxSaveGameSubsystem::ApplySavedPlayerStats(AActor* PlayerActor) const
{
	if (SaveGame && SaveGame->bHasPlayerStats && PlayerActor)
	{
		UWxSaveWorldSubsystem::ApplyPlayerStats(PlayerActor, SaveGame->PlayerStats);
	}
}

void UWxSaveGameSubsystem::ReportTravelFromSaveFileComplete(UWorld* World)
{
	const FName ExpectedMap = SaveGame && !SaveGame->TravelData.Map.IsNull()
		? SaveGame->TravelData.Map.GetAssetPath().GetPackageName()
		: NAME_None;
	const FName LoadedMap = World ? GetStableMapPackageName(World) : NAME_None;

	UE_CLOG(ExpectedMap.IsNone() || LoadedMap == ExpectedMap, LogWxSave, Log,
		TEXT("저장 파일 트래블 완료: '%s'"), *LoadedMap.ToString());
	UE_CLOG(!ExpectedMap.IsNone() && LoadedMap != ExpectedMap, LogWxSave, Warning,
		TEXT("저장 파일 트래블 맵 불일치: 기대 '%s', 실제 '%s'"), *ExpectedMap.ToString(), *LoadedMap.ToString());
	bTravelingFromSaveFile = false;
}

FName UWxSaveGameSubsystem::GetStableMapPackageName(const UWorld* World)
{
	return World ? FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName())) : NAME_None;
}

void UWxSaveGameSubsystem::LogSaveState() const
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump] 활성 SaveGame 없음"));
		return;
	}

	UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump] 슬롯 '%s' · 포맷 %d · 맵 상태 %d개 · 트래블 맵 %s"),
		*SaveGame->SlotName,
		SaveGame->SaveFormatVersion,
		SaveGame->SavedStatePerMap.Num(),
		SaveGame->TravelData.Map.IsNull() ? TEXT("(미기록)") : *SaveGame->TravelData.Map.ToString());

	for (const TPair<FName, FWxWorldPersistenceEntry>& Pair : SaveGame->SavedStatePerMap)
	{
		UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump]   %s: LSP %d바이트 · IA 레벨 %d개 · Mass 그룹 %d개"),
			*Pair.Key.ToString(),
			Pair.Value.StreamingLevelData.Num(),
			Pair.Value.SavedStatePerStreamingLevel.Num(),
			Pair.Value.MassEntitySnapshots.Num());
	}
}

bool UWxSaveGameSubsystem::IsSaveInProgress() const
{
	return bSaveInProgress;
}

void UWxSaveGameSubsystem::HandleSaveFlushComplete()
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("디스크 저장 중단: 활성 SaveGame 없음"));
		FinishSaveInProgress();
		return;
	}

	UGameplayStatics::AsyncSaveGameToSlot(
		SaveGame,
		SaveGame->SlotName,
		SaveGame->UserIndex,
		FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UWxSaveGameSubsystem::HandleAsyncSaveFinished));
}

void UWxSaveGameSubsystem::HandleAsyncSaveFinished(const FString& SlotName, int32 UserIndex, bool bSuccess)
{
	FinishSaveInProgress();
	if (bSuccess)
	{
		UE_LOG(LogWxSave, Log, TEXT("슬롯 '%s' 디스크 저장 성공"), *SlotName);
	}
	else
	{
		UE_LOG(LogWxSave, Warning, TEXT("슬롯 '%s' 디스크 저장 실패"), *SlotName);
	}
}

void UWxSaveGameSubsystem::FinishSaveInProgress()
{
	bSaveInProgress = false;
	OnSaveCompleted.Broadcast();
	OnSaveCompleted.Clear();
}

int32 UWxSaveGameSubsystem::GetCurrentSaveFormatVersion()
{
	return 3;
}
