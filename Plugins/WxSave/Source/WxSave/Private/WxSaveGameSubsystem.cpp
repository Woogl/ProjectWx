// Copyright Woogle. All Rights Reserved.

#include "WxSaveGameSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "WxSaveWorldSubsystem.h"
#include "WxSaveModule.h"

static FAutoConsoleCommandWithWorld GWxSaveDumpCommand(
	TEXT("Wx.Save.Dump"),
	TEXT("현재 WxSave 메모리 슬롯 상태(슬롯·맵·폰·레코드 목록)를 로그로 덤프한다."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		if (UWxSaveGameSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr)
		{
			Subsystem->LogSaveState();
		}
		else
		{
			UE_LOG(LogWxSave, Warning, TEXT("Wx.Save.Dump: WxSaveGameSubsystem 을 찾을 수 없음"));
		}
	}));

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

UWxSaveGame* UWxSaveGameSubsystem::StartNewSaveFile(const FString& SlotName, int32 UserIndex, TSubclassOf<UWxSaveGame> SpecificClass)
{
	if (!SpecificClass)
	{
		UE_LOG(LogWxSave, Warning, TEXT("StartNewSaveFile: SpecificClass 가 null — UWxSaveGame 서브클래스를 지정해야 한다."));
		return nullptr;
	}

	UWxSaveGame* NewSaveGame = Cast<UWxSaveGame>(UGameplayStatics::CreateSaveGameObject(SpecificClass));
	if (!NewSaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("StartNewSaveFile: '%s' SaveGame 생성 실패"), *SpecificClass->GetName());
		return nullptr;
	}

	NewSaveGame->SlotName = SlotName;
	NewSaveGame->UserIndex = UserIndex;

	SaveGame = NewSaveGame;
	UE_LOG(LogWxSave, Log, TEXT("StartNewSaveFile: 슬롯 '%s' (UserIndex %d) 새 SaveGame 시작"), *SaveGame->SlotName, SaveGame->UserIndex);
	return SaveGame;
}

UWxSaveGame* UWxSaveGameSubsystem::LoadFromFile(const FString& SlotName, int32 UserIndex, bool bStartTravel)
{
	// 값 복사라 아래에서 SaveGame 이 교체돼도 안전하다.
	FString TargetSlot = SlotName;
	int32 TargetUserIndex = UserIndex;
	if (TargetSlot.IsEmpty() && SaveGame)
	{
		TargetSlot = SaveGame->SlotName;
		TargetUserIndex = SaveGame->UserIndex;
	}

	if (UWxSaveGame* Loaded = Cast<UWxSaveGame>(UGameplayStatics::LoadGameFromSlot(TargetSlot, TargetUserIndex)))
	{
		SaveGame = Loaded;
		// 구버전 파일엔 슬롯 정체성이 없을 수 있으므로 로드 경로의 값으로 보정한다.
		SaveGame->SlotName = TargetSlot;
		SaveGame->UserIndex = TargetUserIndex;
		UE_LOG(LogWxSave, Log, TEXT("LoadFromFile: 슬롯 '%s' 로드 — 레코드 %d개"), *TargetSlot, SaveGame->ActorRecords.Num());
	}
	else
	{
		// 이전 세션의 잔여 상태가 남지 않도록 빈 슬롯으로 리셋한다.
		StartNewSaveFile(TargetSlot, TargetUserIndex, UWxSaveGame::StaticClass());
		UE_LOG(LogWxSave, Log, TEXT("LoadFromFile: 슬롯 '%s' 파일 없음/손상 — 빈 슬롯으로 시작"), *TargetSlot);
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
		UE_LOG(LogWxSave, Warning, TEXT("TravelFromSaveFile: 활성 SaveGame 없음 — LoadFromFile 또는 StartNewSaveFile 을 먼저 호출하라."));
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogWxSave, Warning, TEXT("TravelFromSaveFile: 월드 없음 — 트래블 불가"));
		return;
	}

	const FSoftObjectPath& Map = SaveGame->TravelData.Map;
	const FString TravelMap = Map.IsNull()
		? GetStableMapPackageName(World).ToString() : Map.GetAssetPath().GetPackageName().ToString();

	// 트래블/teardown 동안의 자동 캡처(teardown 플러시·스트리밍-아웃)가 방금 로드한 세이브를 라이브 상태로 덮어쓰지 않도록 가드를 건다.
	bTravelingFromSaveFile = true;

	// 트래블은 스탠드얼론 전제의 기존 검증 경로인 ServerTravel(bAbsolute)을 유지한다(샘플의 OpenLevel 은 클라 트래블 의미론일 뿐 기술적 우위 없음).
	UE_LOG(LogWxSave, Log, TEXT("TravelFromSaveFile: '%s' 로 트래블(ServerTravel)"), *TravelMap);
	if (!World->ServerTravel(TravelMap, true))
	{
		// 가드를 즉시 해제해 자동 캡처 경로를 복구한다.
		bTravelingFromSaveFile = false;
		UE_LOG(LogWxSave, Warning, TEXT("TravelFromSaveFile: ServerTravel 시작 실패 — '%s'"), *TravelMap);
	}
}

void UWxSaveGameSubsystem::SaveToFile(const FString& SlotName, int32 UserIndex, const FTransform* ResumeTransform)
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("SaveToFile: 활성 SaveGame 없음 — StartNewSaveFile 을 먼저 호출하라."));
		return;
	}

	// 재지정은 플러시(FlushMapTravelData 등 — 슬롯 정체성 무관)와 디스크 기록(SaveGame->SlotName 사용) 사이에서 안전하다.
	if (!SlotName.IsEmpty())
	{
		SaveGame->SlotName = SlotName;
		SaveGame->UserIndex = UserIndex;
	}

	// 아래 경로 중 어디로 가든 ContinueSaveToFileToDisk 가 내린다.
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
			UWxSaveWorldSubsystem::FOnSaveFlushComplete::FDelegate::CreateUObject(this, &UWxSaveGameSubsystem::ContinueSaveToFileToDisk),
			ResumeTransform);
	}
	else
	{
		ContinueSaveToFileToDisk();
	}
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
		UE_LOG(LogWxSave, Log, TEXT("DeleteSaveFile: 슬롯 '%s' (UserIndex %d) 삭제"), *SlotName, UserIndex);
	}
	else
	{
		UE_LOG(LogWxSave, Warning, TEXT("DeleteSaveFile: 슬롯 '%s' (UserIndex %d) 삭제 실패(파일 없음 등)"), *SlotName, UserIndex);
	}
	return bDeleted;
}

void UWxSaveGameSubsystem::SetTravelData(FWxSaveTravelData InTravelData)
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("SetTravelData: 활성 SaveGame 없음 — StartNewSaveFile 을 먼저 호출하라."));
		return;
	}

	SaveGame->TravelData = MoveTemp(InTravelData);
}

bool UWxSaveGameSubsystem::TryGetPlayerTransform(const UWorld* World, FTransform& OutTransform) const
{
	if (!SaveGame || !World)
	{
		return false;
	}

	// 맵 일치 판정은 ReportTravelFromSaveFileComplete 와 같은 비교다.
	const FName SavedMap = SaveGame->TravelData.Map.IsNull() ? NAME_None : SaveGame->TravelData.Map.GetAssetPath().GetPackageName();
	const FName CurrentMap = GetStableMapPackageName(World);
	if (SaveGame->PlayerTransform.Equals(FTransform::Identity) || SavedMap != CurrentMap)
	{
		return false;
	}

	OutTransform = SaveGame->PlayerTransform;
	return true;
}

void UWxSaveGameSubsystem::ApplySavedPlayerStats(AActor* PlayerActor) const
{
	if (!SaveGame || !SaveGame->bHasPlayerStats || !PlayerActor)
	{
		return;
	}

	UWxSaveWorldSubsystem::ApplyPlayerStats(PlayerActor, SaveGame->PlayerStats);
}

void UWxSaveGameSubsystem::ReportTravelFromSaveFileComplete(UWorld* World)
{
	const FName ExpectedMap = SaveGame && !SaveGame->TravelData.Map.IsNull()
		? SaveGame->TravelData.Map.GetAssetPath().GetPackageName() : NAME_None;
	const FName LoadedMap = World ? GetStableMapPackageName(World) : NAME_None;

	if (ExpectedMap.IsNone() || LoadedMap == ExpectedMap)
	{
		UE_LOG(LogWxSave, Log, TEXT("ReportTravelFromSaveFileComplete: '%s' 에서 트래블 완료"), *LoadedMap.ToString());
	}
	else
	{
		UE_LOG(LogWxSave, Warning, TEXT("ReportTravelFromSaveFileComplete: 기대 맵 '%s' 이 아닌 '%s' 에서 보고됨 — 복원이 불완전할 수 있다."), *ExpectedMap.ToString(), *LoadedMap.ToString());
	}

	bTravelingFromSaveFile = false;
}

FName UWxSaveGameSubsystem::GetStableMapPackageName(const UWorld* World)
{
	// 엔진 LoadMap 이 트래블 시 PIE 접두사를 제거/재부여하므로 이 표현이 PIE/스탠드얼론 공통으로 안전하다.
	return FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
}

void UWxSaveGameSubsystem::LogSaveState() const
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump] 활성 SaveGame 없음"));
		return;
	}

	const bool bHasPlayerTransform = !SaveGame->PlayerTransform.Equals(FTransform::Identity);
	UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump] 슬롯 '%s' · 레코드 %d개 · 저장 맵 %s · 재개 위치 %s"),
		*SaveGame->SlotName,
		SaveGame->ActorRecords.Num(),
		SaveGame->TravelData.Map.IsNull() ? TEXT("(미기록)") : *SaveGame->TravelData.Map.ToString(),
		bHasPlayerTransform ? *SaveGame->PlayerTransform.GetLocation().ToString() : TEXT("(미설정)"));

	for (const TPair<FGuid, FWxActorRecord>& Pair : SaveGame->ActorRecords)
	{
		UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump]   %s : 액터바이트 %d · 컴포넌트 %d개 · 버전헤더 %d바이트"),
			*Pair.Key.ToString(), Pair.Value.ByteData.Num(), Pair.Value.ComponentData.Num(), Pair.Value.VersionHeader.Num());
	}
}

bool UWxSaveGameSubsystem::IsSaveInProgress() const
{
	return bSaveInProgress;
}

void UWxSaveGameSubsystem::ContinueSaveToFileToDisk()
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("ContinueSaveToFileToDisk: 활성 SaveGame 없음 — 기록 중단"));
		FinishSaveInProgress();
		return;
	}

	TWeakObjectPtr<UWxSaveGameSubsystem> WeakThis(this);
	UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SaveGame->SlotName, SaveGame->UserIndex,
		FAsyncSaveGameToSlotDelegate::CreateLambda([WeakThis](const FString& Slot, int32 /*UserIndex*/, bool bSuccess)
		{
			if (UWxSaveGameSubsystem* Subsystem = WeakThis.Get())
			{
				Subsystem->FinishSaveInProgress();
			}

			if (bSuccess)
			{
				UE_LOG(LogWxSave, Log, TEXT("SaveToFile 완료: 슬롯 '%s' 디스크 기록 성공"), *Slot);
			}
			else
			{
				UE_LOG(LogWxSave, Warning, TEXT("SaveToFile 실패: 슬롯 '%s' 디스크 기록 실패"), *Slot);
			}
		}));
}

void UWxSaveGameSubsystem::FinishSaveInProgress()
{
	bSaveInProgress = false;

	OnSaveCompleted.Broadcast();
	OnSaveCompleted.Clear();
}
