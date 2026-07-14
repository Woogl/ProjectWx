// Copyright Woogle. All Rights Reserved.

#include "WxPersistenceGameSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "WxPersistenceWorldSubsystem.h"
#include "WxSaveModule.h"

// 현재 GameInstance 의 WxPersistence 슬롯 상태를 로그로 덤프하는 디버그 콘솔 명령.
static FAutoConsoleCommandWithWorld GWxSaveDumpCommand(
	TEXT("Wx.Save.Dump"),
	TEXT("현재 WxSave 메모리 슬롯 상태(슬롯·맵·폰·레코드 목록)를 로그로 덤프한다."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		if (UWxPersistenceGameSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr)
		{
			Subsystem->LogSaveState();
		}
		else
		{
			UE_LOG(LogWxSave, Warning, TEXT("Wx.Save.Dump: WxPersistenceGameSubsystem 을 찾을 수 없음"));
		}
	}));

void UWxPersistenceGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// PIE·스탠드얼론·패키지 모두 빈 새 SaveGame 으로 시작한다("신선한 시작" 의미론). 슬롯 이름은 체크포인트 오토세이브·UI 로드가 쓸 디스크 파일명이며, 이후 로드는 UI 의 LoadFromFile 몫이다.
	StartNewSaveFile(TEXT("Test"), 0, UWxPersistenceSaveGame::StaticClass());
}

UWxPersistenceSaveGame* UWxPersistenceGameSubsystem::StartNewSaveFile(const FString& SlotName, int32 UserIndex, TSubclassOf<UWxPersistenceSaveGame> SpecificClass)
{
	if (!SpecificClass)
	{
		UE_LOG(LogWxSave, Warning, TEXT("StartNewSaveFile: SpecificClass 가 null — UWxPersistenceSaveGame 서브클래스를 지정해야 한다."));
		return nullptr;
	}

	UWxPersistenceSaveGame* NewSaveGame = Cast<UWxPersistenceSaveGame>(UGameplayStatics::CreateSaveGameObject(SpecificClass));
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

UWxPersistenceSaveGame* UWxPersistenceGameSubsystem::LoadFromFile(const FString& SlotName, int32 UserIndex, bool bStartTravel)
{
	if (UWxPersistenceSaveGame* Loaded = Cast<UWxPersistenceSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex)))
	{
		SaveGame = Loaded;
		// 구버전 파일엔 슬롯 정체성이 없을 수 있으므로 로드 경로의 값으로 보정한다.
		SaveGame->SlotName = SlotName;
		SaveGame->UserIndex = UserIndex;
		UE_LOG(LogWxSave, Log, TEXT("LoadFromFile: 슬롯 '%s' 로드 — 레코드 %d개"), *SlotName, SaveGame->ActorRecords.Num());
	}
	else
	{
		// 파일 부재/손상: 같은 슬롯의 빈 SaveGame 으로 리셋한다(이전 세션 잔여 상태 차단).
		// 샘플은 여기서 중단하지만, Wx 는 사망 리스폰(WBP_DeathScreen)이 파일 없이도 월드 리로드에 의존하므로 리셋 후에도 트래블을 이어간다.
		StartNewSaveFile(SlotName, UserIndex, UWxPersistenceSaveGame::StaticClass());
		UE_LOG(LogWxSave, Log, TEXT("LoadFromFile: 슬롯 '%s' 파일 없음/손상 — 빈 슬롯으로 시작"), *SlotName);
	}

	if (bStartTravel)
	{
		TravelFromSaveFile();
	}

	return SaveGame;
}

UWxPersistenceSaveGame* UWxPersistenceGameSubsystem::ReloadFromFile(bool bStartTravel)
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("ReloadFromFile: 활성 SaveGame 없음 — LoadFromFile 또는 StartNewSaveFile 을 먼저 호출하라."));
		return nullptr;
	}

	return LoadFromFile(SaveGame->SlotName, SaveGame->UserIndex, bStartTravel);
}

void UWxPersistenceGameSubsystem::TravelFromSaveFile()
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

	// 저장된 맵이 있으면 그 맵으로, 없으면(구버전 파일/신규 슬롯) 현재 맵 리로드로 폴백한다(샘플은 중단 — Wx 는 사망 리스폰이 리로드에 의존).
	const FSoftObjectPath& Map = SaveGame->TravelData.Map;
	const FString TravelMap = Map.IsNull()
		? GetStableMapPackageName(World).ToString() : Map.GetAssetPath().GetPackageName().ToString();

	// 트래블/teardown 동안의 자동 캡처(teardown 플러시·스트리밍-아웃)가 방금 로드한 세이브를 라이브 상태로 덮어쓰지 않도록 가드를 건다.
	bTravelingFromSaveFile = true;

	// 트래블은 스탠드얼론 전제의 기존 검증 경로인 ServerTravel(bAbsolute)을 유지한다(샘플의 OpenLevel 은 클라 트래블 의미론일 뿐 기술적 우위 없음).
	UE_LOG(LogWxSave, Log, TEXT("TravelFromSaveFile: '%s' 로 트래블(ServerTravel)"), *TravelMap);
	if (!World->ServerTravel(TravelMap, true))
	{
		// 트래블 시작 실패(맵 이름 무효 등): 가드를 즉시 해제해 자동 캡처 경로를 복구한다.
		bTravelingFromSaveFile = false;
		UE_LOG(LogWxSave, Warning, TEXT("TravelFromSaveFile: ServerTravel 시작 실패 — '%s'"), *TravelMap);
	}
}

void UWxPersistenceGameSubsystem::SaveToFile(const FString& SlotName, int32 UserIndex)
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("SaveToFile: 활성 SaveGame 없음 — StartNewSaveFile 을 먼저 호출하라."));
		return;
	}

	// 슬롯 이름이 지정되면(=명시 저장) 활성 슬롯을 그 이름으로 재지정한다(이후 체크포인트 오토세이브도 이 슬롯을 이어감). 비면(=오토세이브) 현재 활성 슬롯을 유지한다.
	// 재지정은 플러시(FlushMapTravelData 등 — 슬롯 정체성 무관)와 디스크 기록(SaveGame->SlotName 사용) 사이에서 안전하다.
	if (!SlotName.IsEmpty())
	{
		SaveGame->SlotName = SlotName;
		SaveGame->UserIndex = UserIndex;
	}

	UWxPersistenceWorldSubsystem* WorldSubsystem = nullptr;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWorld* World = GameInstance->GetWorld())
		{
			WorldSubsystem = World->GetSubsystem<UWxPersistenceWorldSubsystem>();
		}
	}

	if (WorldSubsystem)
	{
		// 라이브 상태(트래블 데이터 + 스탯 + savable 액터)를 SaveGame 에 플러시한 뒤 완료 콜백으로 디스크에 기록한다(현재 전부 동기 — 콜백은 즉시 발화).
		// 부활 지점(RespawnTransform)은 체크포인트가 이미 세팅해 둔 값을 그대로 영속한다.
		WorldSubsystem->RequestSaveFlush(
			UWxPersistenceWorldSubsystem::FOnSaveFlushComplete::FDelegate::CreateUObject(this, &UWxPersistenceGameSubsystem::ContinueSaveToFileToDisk));
	}
	else
	{
		// 월드 서브시스템 부재(트랜지션 등): 플러시할 것이 없으니 바로 기록한다.
		ContinueSaveToFileToDisk();
	}
}

bool UWxPersistenceGameSubsystem::DoesSaveFileExist(const FString& SlotName, int32 UserIndex) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

bool UWxPersistenceGameSubsystem::DeleteSaveFile(const FString& SlotName, int32 UserIndex)
{
	// 디스크 파일만 삭제한다. 인메모리 활성 SaveGame 은 그대로 두므로, 활성 슬롯을 지웠다면 다음 SaveToFile 이 그 파일을 다시 만든다.
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

void UWxPersistenceGameSubsystem::SetPersistenceTravelData(FWxPersistenceTravelData InTravelData)
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("SetPersistenceTravelData: 활성 SaveGame 없음 — StartNewSaveFile 을 먼저 호출하라."));
		return;
	}

	SaveGame->TravelData = MoveTemp(InTravelData);
}

void UWxPersistenceGameSubsystem::SetRespawnTransform(const FTransform& InRespawnTransform)
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("SetRespawnTransform: 활성 SaveGame 없음 — StartNewSaveFile 을 먼저 호출하라."));
		return;
	}

	SaveGame->RespawnTransform = InRespawnTransform;
}

FTransform UWxPersistenceGameSubsystem::GetRespawnTransform() const
{
	return SaveGame ? SaveGame->RespawnTransform : FTransform::Identity;
}

void UWxPersistenceGameSubsystem::ReportTravelFromSaveFileComplete(UWorld* World)
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

FName UWxPersistenceGameSubsystem::GetStableMapPackageName(const UWorld* World)
{
	// PIE 의 UEDPIE_N_ 접두사를 제거한 긴 패키지 이름. 엔진 LoadMap 이 트래블 시 접두사를 제거/재부여하므로 이 표현이 PIE/스탠드얼론 공통으로 안전하다.
	return FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
}

void UWxPersistenceGameSubsystem::LogSaveState() const
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump] 활성 SaveGame 없음"));
		return;
	}

	const bool bHasRespawn = !SaveGame->RespawnTransform.Equals(FTransform::Identity);
	UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump] 슬롯 '%s' · 레코드 %d개 · 저장 맵 %s · 부활 위치 %s"),
		*SaveGame->SlotName,
		SaveGame->ActorRecords.Num(),
		SaveGame->TravelData.Map.IsNull() ? TEXT("(미기록)") : *SaveGame->TravelData.Map.ToString(),
		bHasRespawn ? *SaveGame->RespawnTransform.GetLocation().ToString() : TEXT("(미설정)"));

	for (const TPair<FGuid, FWxActorRecord>& Pair : SaveGame->ActorRecords)
	{
		UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump]   %s : 액터바이트 %d · 컴포넌트 %d개 · 버전헤더 %d바이트"),
			*Pair.Key.ToString(), Pair.Value.ByteData.Num(), Pair.Value.ComponentData.Num(), Pair.Value.VersionHeader.Num());
	}
}

void UWxPersistenceGameSubsystem::ContinueSaveToFileToDisk()
{
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("ContinueSaveToFileToDisk: 활성 SaveGame 없음 — 기록 중단"));
		return;
	}

	// 직렬화는 게임 스레드에서 동기 수행되고 디스크 쓰기만 비동기다. 완료 델리게이트로 기록 성공/실패를 가시화한다.
	UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SaveGame->SlotName, SaveGame->UserIndex,
		FAsyncSaveGameToSlotDelegate::CreateLambda([](const FString& Slot, int32 /*UserIndex*/, bool bSuccess)
		{
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
