// Copyright Woogle. All Rights Reserved.

#include "WxSaveGameSubsystem.h"

#include "Components/ActorComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "WxSavableInterface.h"
#include "WxSaveGame.h"
#include "WxSaveModule.h"

// 현재 GameInstance 의 WxSaveGameSubsystem 슬롯 상태를 로그로 덤프하는 디버그 콘솔 명령.
static FAutoConsoleCommandWithWorld GWxSaveDumpCommand(
	TEXT("Wx.Save.Dump"),
	TEXT("현재 WxSave 메모리 슬롯 상태(레코드 키 + 부활 Transform)를 로그로 덤프한다."),
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

void UWxSaveGameSubsystem::SaveSlot(const FString& SlotName)
{
	EnsureSaveObject();

	int32 SavableCount = 0;

	// 현재 월드의 IWxSavable 액터를 캡처. 스트리밍-아웃 셀의 액터는 이미 LevelRemovedFromWorld 에서 기록됨.
	if (const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->Implements<UWxSavableInterface>())
			{
				CaptureActor(Actor);
				++SavableCount;
			}
		}

		// 플레이어 Pawn 은 IWxSavable 이 아니므로(런타임 스폰이라 키가 불안정) Transform 만 별도 캡처.
		// LoadSlot 시 ChoosePlayerStart 에서 부활 진입점으로 사용된다.
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (const APawn* PlayerPawn = PC->GetPawn())
			{
				CurrentSave->PlayerRespawnTransform = PlayerPawn->GetActorTransform();
			}
		}
	}

	UE_LOG(LogWxSave, Log, TEXT("SaveSlot 시작: 슬롯 '%s' — IWxSavable %d개 캡처, 누적 레코드 %d개"),
		*SlotName, SavableCount, CurrentSave->ActorRecords.Num());

	// 직렬화는 게임 스레드에서 동기 수행되고 디스크 쓰기만 비동기다. 완료 델리게이트로 기록 성공/실패를 가시화한다.
	UGameplayStatics::AsyncSaveGameToSlot(CurrentSave, SlotName, 0,
		FAsyncSaveGameToSlotDelegate::CreateLambda([](const FString& Slot, int32 /*UserIndex*/, bool bSuccess)
		{
			if (bSuccess)
			{
				UE_LOG(LogWxSave, Log, TEXT("SaveSlot 완료: 슬롯 '%s' 디스크 기록 성공"), *Slot);
			}
			else
			{
				UE_LOG(LogWxSave, Warning, TEXT("SaveSlot 실패: 슬롯 '%s' 디스크 기록 실패"), *Slot);
			}
		}));
}

bool UWxSaveGameSubsystem::LoadSlot(const FString& SlotName)
{
	const bool bFileExists = UGameplayStatics::DoesSaveGameExist(SlotName, 0);
	if (bFileExists)
	{
		if (UWxSaveGame* Loaded = Cast<UWxSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
		{
			CurrentSave = Loaded;
			UE_LOG(LogWxSave, Log, TEXT("LoadSlot: 슬롯 '%s' 로드 — 레코드 %d개"), *SlotName, CurrentSave->ActorRecords.Num());
		}
		else
		{
			// 캐스트 실패 (잘못된 클래스 등): 빈 슬롯으로 안전 리셋.
			CurrentSave = Cast<UWxSaveGame>(UGameplayStatics::CreateSaveGameObject(UWxSaveGame::StaticClass()));
			UE_LOG(LogWxSave, Warning, TEXT("LoadSlot: 슬롯 '%s' 파일이 UWxSaveGame 으로 캐스트되지 않음(손상/클래스 불일치) — 빈 슬롯으로 리셋"), *SlotName);
		}
	}
	else
	{
		// 슬롯 파일 부재: 메모리를 빈 슬롯으로 리셋 (이전 세션 잔여 상태 차단).
		CurrentSave = Cast<UWxSaveGame>(UGameplayStatics::CreateSaveGameObject(UWxSaveGame::StaticClass()));
		UE_LOG(LogWxSave, Log, TEXT("LoadSlot: 슬롯 '%s' 파일 없음 — 빈 슬롯으로 시작"), *SlotName);
	}

	// 월드 리로드로 부활: ServerTravel 후
	//  - 새 월드의 OnWorldInitializedActors 핸들러가 IWxSavable 액터에 ActorRecords 자동 복원
	//  - 새 GameMode 의 ChoosePlayerStart → WxGameMode 가 PlayerRespawnTransform 으로 임시 PlayerStart 생성
	//  - 새 Pawn 이 그 위치에 fresh 상태(HP 풀, 죽음 태그 0)로 스폰
	// GameInstanceSubsystem 이라 CurrentSave 가 트래블을 가로질러 유지된다.
	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		UE_LOG(LogWxSave, Log, TEXT("LoadSlot: 맵 '%s' 리로드(ServerTravel)"), *World->GetMapName());
		World->ServerTravel(World->GetMapName(), true);
	}

	return bFileExists;
}

bool UWxSaveGameSubsystem::GetPlayerRespawnTransform(FTransform& OutTransform) const
{
	if (!CurrentSave || CurrentSave->PlayerRespawnTransform.Equals(FTransform::Identity))
	{
		return false;
	}

	OutTransform = CurrentSave->PlayerRespawnTransform;
	return true;
}

void UWxSaveGameSubsystem::LogSaveState() const
{
	if (!CurrentSave)
	{
		UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump] CurrentSave 없음"));
		return;
	}

	const bool bHasRespawn = !CurrentSave->PlayerRespawnTransform.Equals(FTransform::Identity);
	UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump] 레코드 %d개 · 부활 위치 %s"),
		CurrentSave->ActorRecords.Num(),
		bHasRespawn ? *CurrentSave->PlayerRespawnTransform.GetLocation().ToString() : TEXT("(미설정)"));

	for (const TPair<FGuid, FWxActorRecord>& Pair : CurrentSave->ActorRecords)
	{
		UE_LOG(LogWxSave, Display, TEXT("[Wx.Save.Dump]   %s : 액터바이트 %d · 컴포넌트 %d개"),
			*Pair.Key.ToString(), Pair.Value.ByteData.Num(), Pair.Value.ComponentData.Num());
	}
}

void UWxSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EnsureSaveObject();

	WorldInitializedActorsHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UWxSaveGameSubsystem::HandleWorldInitializedActors);
	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UWxSaveGameSubsystem::HandleLevelAddedToWorld);
	LevelRemovedHandle = FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &UWxSaveGameSubsystem::HandleLevelRemovedFromWorld);
}

void UWxSaveGameSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitializedActorsHandle);
	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);
	FWorldDelegates::LevelRemovedFromWorld.Remove(LevelRemovedHandle);

	Super::Deinitialize();
}

void UWxSaveGameSubsystem::EnsureSaveObject()
{
	if (!CurrentSave)
	{
		CurrentSave = Cast<UWxSaveGame>(UGameplayStatics::CreateSaveGameObject(UWxSaveGame::StaticClass()));
	}
}

void UWxSaveGameSubsystem::CaptureActor(AActor* Actor)
{
	const IWxSavableInterface* Savable = Cast<IWxSavableInterface>(Actor);
	if (!Savable)
	{
		return;
	}

	const FGuid ActorId = Savable->GetWxSaveId();
	if (!ActorId.IsValid())
	{
		UE_LOG(LogWxSave, Warning, TEXT("CaptureActor: '%s' 의 WxSaveId 가 유효하지 않아 저장에서 제외됨. 에디터에서 WxSaveId 부여 경로(PostLoad 등)가 동작했는지 확인하라."), *GetNameSafe(Actor));
		return;
	}

	FWxActorRecord& Record = CurrentSave->ActorRecords.FindOrAdd(ActorId);
	Record.Transform = Actor->GetActorTransform();
	Record.ByteData.Reset();
	Record.ComponentData.Reset();

	{
		FMemoryWriter MemWriter(Record.ByteData, true);
		FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
		Ar.ArIsSaveGame = true;
		Actor->Serialize(Ar);
	}

	// 컴포넌트의 UPROPERTY(SaveGame) 필드는 Actor::Serialize 가 자동으로 끌고 가지 않으므로 컴포넌트 FName 으로 별도 캡처.
	for (UActorComponent* Component : Actor->GetComponents())
	{
		if (!Component)
		{
			continue;
		}

		FWxComponentRecord& ComponentRecord = Record.ComponentData.FindOrAdd(Component->GetFName());
		ComponentRecord.ByteData.Reset();

		FMemoryWriter MemWriter(ComponentRecord.ByteData, true);
		FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
		Ar.ArIsSaveGame = true;
		Component->Serialize(Ar);
	}
}

bool UWxSaveGameSubsystem::RestoreActor(AActor* Actor)
{
	IWxSavableInterface* Savable = Cast<IWxSavableInterface>(Actor);
	if (!Savable)
	{
		return false;
	}

	const FGuid ActorId = Savable->GetWxSaveId();
	if (!ActorId.IsValid())
	{
		UE_LOG(LogWxSave, Warning, TEXT("RestoreActor: '%s' 의 WxSaveId 가 유효하지 않아 복원할 수 없음."), *GetNameSafe(Actor));
		return false;
	}

	const FWxActorRecord* Record = CurrentSave->ActorRecords.Find(ActorId);
	if (!Record)
	{
		// 일치 레코드 없음 — 신규 세션/미저장 액터의 정상 경로.
		return false;
	}

	Actor->SetActorTransform(Record->Transform);

	{
		FMemoryReader MemReader(Record->ByteData, true);
		FObjectAndNameAsStringProxyArchive Ar(MemReader, false);
		Ar.ArIsSaveGame = true;
		Actor->Serialize(Ar);
	}

	for (UActorComponent* Component : Actor->GetComponents())
	{
		if (!Component)
		{
			continue;
		}

		const FWxComponentRecord* ComponentRecord = Record->ComponentData.Find(Component->GetFName());
		if (!ComponentRecord || ComponentRecord->ByteData.Num() == 0)
		{
			continue;
		}

		FMemoryReader MemReader(ComponentRecord->ByteData, true);
		FObjectAndNameAsStringProxyArchive Ar(MemReader, false);
		Ar.ArIsSaveGame = true;
		Component->Serialize(Ar);
	}

	Savable->OnWxSaveRestored();

	UE_LOG(LogWxSave, Verbose, TEXT("RestoreActor: '%s' (%s) 복원"), *GetNameSafe(Actor), *ActorId.ToString());
	return true;
}

bool UWxSaveGameSubsystem::IsOwnedGameWorld(const UWorld* World) const
{
	return World && World->IsGameWorld() && World->GetGameInstance() == GetGameInstance();
}

void UWxSaveGameSubsystem::HandleWorldInitializedActors(const UWorld::FActorsInitializedParams& Params)
{
	if (!IsOwnedGameWorld(Params.World))
	{
		return;
	}

	// 영구 레벨 + 초기 WP 셀 액터에 메모리 ActorRecords 자동 복원. 신규 세션이면 메모리가 비어있어 noop.
	int32 SavableCount = 0;
	int32 RestoredCount = 0;
	for (TActorIterator<AActor> It(Params.World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->Implements<UWxSavableInterface>())
		{
			++SavableCount;
			RestoredCount += RestoreActor(Actor) ? 1 : 0;
		}
	}

	UE_LOG(LogWxSave, Log, TEXT("월드 초기화 복원: IWxSavable %d개 중 %d개에 슬롯 적용 (슬롯 레코드 %d개)"),
		SavableCount, RestoredCount, CurrentSave ? CurrentSave->ActorRecords.Num() : 0);
}

void UWxSaveGameSubsystem::HandleLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	if (!Level || !IsOwnedGameWorld(World))
	{
		return;
	}

	// 스트리밍-인 셀(World Partition cell 포함) 액터에 메모리 ActorRecords 자동 복원.
	int32 RestoredCount = 0;
	for (AActor* Actor : Level->Actors)
	{
		if (Actor && Actor->Implements<UWxSavableInterface>())
		{
			RestoredCount += RestoreActor(Actor) ? 1 : 0;
		}
	}

	UE_LOG(LogWxSave, Verbose, TEXT("스트리밍-인 복원: 레벨 '%s' — %d개 복원"), *Level->GetOutermost()->GetName(), RestoredCount);
}

void UWxSaveGameSubsystem::HandleLevelRemovedFromWorld(ULevel* Level, UWorld* World)
{
	if (!Level || !IsOwnedGameWorld(World))
	{
		return;
	}

	// 스트리밍-아웃 직전 자동 캡처: 셀 왕복으로 인한 상태 손실 방지.
	EnsureSaveObject();
	int32 CapturedCount = 0;
	for (AActor* Actor : Level->Actors)
	{
		if (Actor && Actor->Implements<UWxSavableInterface>())
		{
			CaptureActor(Actor);
			++CapturedCount;
		}
	}

	UE_LOG(LogWxSave, Verbose, TEXT("스트리밍-아웃 캡처: 레벨 '%s' — IWxSavable %d개"), *Level->GetOutermost()->GetName(), CapturedCount);
}
