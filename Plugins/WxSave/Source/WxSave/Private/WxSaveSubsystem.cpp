// Copyright Woogle. All Rights Reserved.

#include "WxSaveSubsystem.h"

#include "Components/ActorComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "WxPlayerSave.h"
#include "WxSavableInterface.h"
#include "WxSaveDeveloperSettings.h"

UWxSaveSubsystem* UWxSaveSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UWxSaveSubsystem>() : nullptr;
}

void UWxSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadOrCreateInMemorySave();

	WorldInitializedActorsHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UWxSaveSubsystem::HandleWorldInitializedActors);
	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UWxSaveSubsystem::HandleLevelAddedToWorld);
}

void UWxSaveSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitializedActorsHandle);
	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);

	Super::Deinitialize();
}

void UWxSaveSubsystem::SaveGame(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	UWorld* World = PC->GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	if (!CurrentSave)
	{
		LoadOrCreateInMemorySave();
	}

	if (const APawn* Pawn = PC->GetPawn())
	{
		CurrentSave->PlayerTransform = Pawn->GetActorTransform();
		CurrentSave->bHasSavedLocation = true;
	}

	CurrentSave->MapName = UGameplayStatics::GetCurrentLevelName(World, true);

	CaptureWorldState(World);

	const FString SlotName = GetDefault<UWxSaveDeveloperSettings>()->PlayerSlotName;
	UGameplayStatics::AsyncSaveGameToSlot(CurrentSave, SlotName, 0);
}

bool UWxSaveSubsystem::RestartFromSave(APlayerController* PC)
{
	if (!PC)
	{
		return false;
	}

	UWorld* World = PC->GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return false;
	}

	if (!CurrentSave || !CurrentSave->bHasSavedLocation || CurrentSave->MapName.IsEmpty())
	{
		// 저장된 데이터가 없으면 의도적으로 동작하지 않는다. 신규 세션은 GameMode 기본 흐름이 처리.
		return false;
	}

	// 플래그 set 후 슬롯 맵으로 ServerTravel. 재로드 후 OnWorldInitializedActors 가 자동 적용,
	// AWxGameMode 가 IsSaveApplied 를 확인해 모든 PC 를 saved transform 으로 spawn 한다 (SpawnCollisionHandling 이 좌표 분산).
	// GameInstanceSubsystem 이라 플래그는 맵 리로드 가로질러 보존된다.
	// ServerTravel 은 standalone/listen/dedicated 어디서나 접속한 모든 PC 를 새 맵으로 데려온다 (OpenLevel 은 로컬 트래블이라 멀티 세션이 끊긴다).
	bSaveApplied = true;

	World->ServerTravel(CurrentSave->MapName, /*bAbsolute=*/true);
	return true;
}

bool UWxSaveSubsystem::HasSavedGame() const
{
	return CurrentSave && CurrentSave->bHasSavedLocation;
}

bool UWxSaveSubsystem::DeleteSavedGame()
{
	const FString SlotName = GetDefault<UWxSaveDeveloperSettings>()->PlayerSlotName;
	const bool bSlotExisted = UGameplayStatics::DoesSaveGameExist(SlotName, 0);

	bool bDeleted = false;
	if (bSlotExisted)
	{
		bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	}

	// 슬롯 파일 부재여도 메모리 상태는 항상 빈 슬롯으로 초기화 (다음 SaveCheckpoint 가 신규 슬롯을 쓰도록).
	CurrentSave = Cast<UWxPlayerSave>(UGameplayStatics::CreateSaveGameObject(UWxPlayerSave::StaticClass()));
	bSaveApplied = false;

	return bDeleted;
}

void UWxSaveSubsystem::BeginNewGame()
{
	// 슬롯 파일은 보존하되 메모리 상태만 빈 슬롯으로 교체. 다음 SaveGame 호출이 슬롯을 덮어쓴다.
	CurrentSave = Cast<UWxPlayerSave>(UGameplayStatics::CreateSaveGameObject(UWxPlayerSave::StaticClass()));
	bSaveApplied = false;
}

bool UWxSaveSubsystem::IsSaveApplied() const
{
	return bSaveApplied;
}

bool UWxSaveSubsystem::HasSavedLocation() const
{
	return CurrentSave && CurrentSave->bHasSavedLocation;
}

FTransform UWxSaveSubsystem::GetSavedPlayerTransform() const
{
	return CurrentSave ? CurrentSave->PlayerTransform : FTransform::Identity;
}

const UWxPlayerSave* UWxSaveSubsystem::GetCurrentSave() const
{
	return CurrentSave;
}

void UWxSaveSubsystem::LoadOrCreateInMemorySave()
{
	const FString SlotName = GetDefault<UWxSaveDeveloperSettings>()->PlayerSlotName;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		CurrentSave = Cast<UWxPlayerSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	}

	if (!CurrentSave)
	{
		CurrentSave = Cast<UWxPlayerSave>(UGameplayStatics::CreateSaveGameObject(UWxPlayerSave::StaticClass()));
	}
}

void UWxSaveSubsystem::HandleWorldInitializedActors(const UWorld::FActorsInitializedParams& Params)
{
	UWorld* World = Params.World;
	if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_Client)
	{
		// 클라에선 서버 리플리케이션이 권위. 로컬 슬롯을 적용하면 잠시 불일치 상태가 보일 수 있어 스킵.
		return;
	}

	// 수동 로드(RestartFromSave) 가 이미 호출된 세션에서만 자동 적용.
	// 같은 GameInstance 내 월드 전환(Seamless Travel 등) 시 기존 로드 세션을 새 월드에도 이어 적용한다.
	if (!bSaveApplied)
	{
		return;
	}

	ApplyStateToWorld(World);
}

void UWxSaveSubsystem::HandleLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	if (!Level || !World || !World->IsGameWorld() || World->GetNetMode() == NM_Client)
	{
		return;
	}

	// 스트리밍 셀이 새로 들어오는 경로. RestartFromSave 로 이미 슬롯을 적용한 세션에서만 push.
	if (!bSaveApplied)
	{
		return;
	}

	ApplyStateToLevel(Level);
}

void UWxSaveSubsystem::ApplyStateToWorld(UWorld* World)
{
	if (!CurrentSave)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		ApplyStateToActor(*It);
	}
}

void UWxSaveSubsystem::ApplyStateToLevel(ULevel* Level)
{
	if (!CurrentSave)
	{
		return;
	}

	for (AActor* Actor : Level->Actors)
	{
		ApplyStateToActor(Actor);
	}
}

void UWxSaveSubsystem::ApplyStateToActor(AActor* Actor)
{
	if (!Actor || !Actor->Implements<UWxSavableInterface>())
	{
		return;
	}

	const FGuid ActorId = Actor->GetActorGuid();
	if (!ActorId.IsValid())
	{
		return;
	}

	const FWxActorRecord* Record = CurrentSave->ActorRecords.Find(ActorId);
	if (!Record)
	{
		return;
	}

	Actor->SetActorTransform(Record->Transform);

	{
		FMemoryReader MemReader(Record->ByteData, true);
		FObjectAndNameAsStringProxyArchive Ar(MemReader, false);
		Ar.ArIsSaveGame = true;
		Actor->Serialize(Ar);
	}

	// 컴포넌트 안의 UPROPERTY(SaveGame) 필드는 Actor::Serialize 가 자동으로 끌고 가지 않으므로
	// 컴포넌트 FName 으로 record 를 찾아 별도 복원한다. 동적으로 추가된 컴포넌트는 default subobject 와 달리 이름이 안정적이지 않을 수 있다.
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

	if (IWxSavableInterface* Savable = Cast<IWxSavableInterface>(Actor))
	{
		Savable->OnWxSaveRestored();
	}
}

void UWxSaveSubsystem::CaptureWorldState(UWorld* World)
{
	if (!World || !CurrentSave)
	{
		return;
	}

	// 본 월드에 살아 있는 savable 액터들의 현재 상태로 기록 갱신.
	// 다른 월드/슬롯의 기록은 보존 (스트리밍 아웃된 셀의 기록을 잃지 않기 위해).
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		CaptureActorState(*It);
	}
}

void UWxSaveSubsystem::CaptureActorState(AActor* Actor)
{
	if (!Actor || !Actor->Implements<UWxSavableInterface>())
	{
		return;
	}

	const FGuid ActorId = Actor->GetActorGuid();
	if (!ActorId.IsValid())
	{
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

	// 컴포넌트별 SaveGame 필드를 별도 ByteData 로 캡처. Apply 시 동일 FName 으로 매칭한다.
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
