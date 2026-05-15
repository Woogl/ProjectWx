// Copyright Woogle. All Rights Reserved.

#include "WxSaveGameSubsystem.h"

#include "Components/ActorComponent.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "WxSavableInterface.h"
#include "WxSaveGame.h"

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

void UWxSaveGameSubsystem::SaveSlot(const FString& SlotName)
{
	EnsureSaveObject();

	// 현재 월드의 IWxSavable 액터를 캡처. 스트리밍-아웃 셀의 액터는 이미 LevelRemovedFromWorld 에서 기록됨.
	if (const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->Implements<UWxSavableInterface>())
			{
				CaptureActor(Actor);
			}
		}

		// 플레이어 Pawn 은 IWxSavable 이 아니므로(런타임 스폰이라 ActorGuid 불안정) Transform 만 별도 캡처.
		// LoadSlot 시 부활 진입점으로 사용된다.
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (const APawn* PlayerPawn = PC->GetPawn())
			{
				CurrentSave->PlayerRespawnTransform = PlayerPawn->GetActorTransform();
			}
		}
	}

	UGameplayStatics::AsyncSaveGameToSlot(CurrentSave, SlotName, 0);
}

bool UWxSaveGameSubsystem::LoadSlot(const FString& SlotName)
{
	const bool bFileExists = UGameplayStatics::DoesSaveGameExist(SlotName, 0);
	if (bFileExists)
	{
		if (UWxSaveGame* Loaded = Cast<UWxSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
		{
			CurrentSave = Loaded;
		}
		else
		{
			// 캐스트 실패 (잘못된 클래스 등): 빈 슬롯으로 안전 리셋.
			CurrentSave = Cast<UWxSaveGame>(UGameplayStatics::CreateSaveGameObject(UWxSaveGame::StaticClass()));
		}
	}
	else
	{
		// 슬롯 파일 부재: 메모리를 빈 슬롯으로 리셋 (이전 세션 잔여 상태 차단).
		CurrentSave = Cast<UWxSaveGame>(UGameplayStatics::CreateSaveGameObject(UWxSaveGame::StaticClass()));
	}

	// 현재 월드의 IWxSavable 액터에 즉시 복원. 빈 슬롯이면 ActorRecords 조회가 모두 miss 라 noop.
	// 플레이어 Pawn 은 IWxSavable 이 아니므로(런타임 스폰이라 ActorGuid 불안정) 부활 Transform 만 별도 적용.
	if (const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->Implements<UWxSavableInterface>())
			{
				RestoreActor(Actor);
			}
		}

		// 같은 월드에서 LoadSlot 한 경우 GameMode::ChoosePlayerStart 가 호출되지 않으므로,
		// 현재 빙의 중인 Pawn 을 슬롯의 부활 Transform 으로 직접 이동시킨다.
		// ServerTravel 후 LoadSlot 인 경우 Pawn 이 아직 없거나 GameMode 가 이미 PlayerStart 로 배치한 상태라 본 분기 영향 없음.
		if (CurrentSave)
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (APawn* PlayerPawn = PC->GetPawn())
				{
					PlayerPawn->SetActorTransform(CurrentSave->PlayerRespawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
		}
	}

	return bFileExists;
}

bool UWxSaveGameSubsystem::TryGetPlayerRespawnTransform(FTransform& OutTransform) const
{
	if (!CurrentSave || CurrentSave->PlayerRespawnTransform.Equals(FTransform::Identity))
	{
		return false;
	}

	OutTransform = CurrentSave->PlayerRespawnTransform;
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
	for (TActorIterator<AActor> It(Params.World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->Implements<UWxSavableInterface>())
		{
			RestoreActor(Actor);
		}
	}
}

void UWxSaveGameSubsystem::HandleLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	if (!Level || !IsOwnedGameWorld(World))
	{
		return;
	}

	// 스트리밍-인 셀(World Partition cell 포함) 액터에 메모리 ActorRecords 자동 복원.
	for (AActor* Actor : Level->Actors)
	{
		if (Actor && Actor->Implements<UWxSavableInterface>())
		{
			RestoreActor(Actor);
		}
	}
}

void UWxSaveGameSubsystem::HandleLevelRemovedFromWorld(ULevel* Level, UWorld* World)
{
	if (!Level || !IsOwnedGameWorld(World))
	{
		return;
	}

	// 스트리밍-아웃 직전 자동 캡처: 셀 왕복으로 인한 상태 손실 방지.
	EnsureSaveObject();
	for (AActor* Actor : Level->Actors)
	{
		if (Actor && Actor->Implements<UWxSavableInterface>())
		{
			CaptureActor(Actor);
		}
	}
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

void UWxSaveGameSubsystem::RestoreActor(AActor* Actor)
{
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
