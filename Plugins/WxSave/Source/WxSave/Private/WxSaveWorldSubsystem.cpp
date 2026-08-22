// Copyright Woogle. All Rights Reserved.

#include "WxSaveWorldSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AttributeSet.h"
#include "Components/ActorComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Serialization/CustomVersion.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/ObjectVersion.h"
#include "UObject/UnrealType.h"
#include "WxSaveGameSubsystem.h"
#include "WxSaveGame.h"
#include "WxSavable.h"
#include "WxSaveModule.h"

void UWxSaveWorldSubsystem::RequestSaveFlush(FOnSaveFlushComplete::FDelegate OnComplete, const FTransform* ResumeTransform)
{
	// Wx 엔 Mass 같은 페이즈 지연 작업이 없어 플러시가 전부 동기다(샘플은 여기서 Mass 스냅샷을 FrameEnd 로 지연).
	UWorld* World = GetWorld();

	if (World && !World->bIsTearingDown)
	{
		FlushMapTravelData();
		FlushPlayerTransform(ResumeTransform);
		FlushPlayerStats();
	}
	FlushSavableActors();

	OnComplete.ExecuteIfBound();
}

bool UWxSaveWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// 저장은 authority 전용이라 클라이언트 월드는 제외한다.
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && !World->IsNetMode(NM_Client);
}

void UWxSaveWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// FWorldDelegates 는 전역이라 모든 월드에서 발화한다 — 각 핸들러 선두에서 자기 월드만 필터한다(PIE 다중 인스턴스 격리 포함).
	WorldInitializedActorsHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UWxSaveWorldSubsystem::HandleWorldInitializedActors);
	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UWxSaveWorldSubsystem::HandleLevelAddedToWorld);
	LevelRemovedHandle = FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &UWxSaveWorldSubsystem::HandleLevelRemovedFromWorld);
	WorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UWxSaveWorldSubsystem::HandleWorldBeginTearDown);
}

void UWxSaveWorldSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitializedActorsHandle);
	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);
	FWorldDelegates::LevelRemovedFromWorld.Remove(LevelRemovedHandle);
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldBeginTearDownHandle);

	Super::Deinitialize();
}

void UWxSaveWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// 복원(OnWorldInitializedActors)과 구 월드 teardown 이 모두 끝난 뒤라 안전한 가드 해제점이다.
	UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	if (GameSubsystem && GameSubsystem->IsTravelingFromSaveFile())
	{
		GameSubsystem->ReportTravelFromSaveFileComplete(&InWorld);
	}
}

UWxSaveGameSubsystem* UWxSaveWorldSubsystem::GetGameSubsystem() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWxSaveGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	if (!GameSubsystem)
	{
		UE_LOG(LogWxSave, Warning, TEXT("WxSaveGameSubsystem 을 찾을 수 없음 — 이 월드의 세이브 요청은 무시된다."));
	}

	return GameSubsystem;
}

UWxSaveGame* UWxSaveWorldSubsystem::GetActiveSaveGame() const
{
	// 서브시스템 부재는 GetGameSubsystem 이 이미 알렸으므로 여기선 슬롯 부재만 알린다 — 실패 1건당 로그 1줄.
	const UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	if (!GameSubsystem)
	{
		return nullptr;
	}

	UWxSaveGame* SaveGame = GameSubsystem->GetSaveGame();
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("활성 SaveGame 없음 — 세이브 요청 무시"));
	}

	return SaveGame;
}

void UWxSaveWorldSubsystem::FlushMapTravelData()
{
	UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	if (!GameSubsystem)
	{
		return;
	}

	// 재개 지점은 FlushPlayerTransform 이 SaveGame 최상위에 담으므로 TravelData 는 맵만 담는다.
	FWxSaveTravelData TravelData;
	TravelData.Map = FSoftObjectPath(UWxSaveGameSubsystem::GetStableMapPackageName(GetWorld()).ToString());
	GameSubsystem->SetTravelData(MoveTemp(TravelData));
}

void UWxSaveWorldSubsystem::FlushSavableActors()
{
	UWxSaveGame* SaveGame = GetActiveSaveGame();
	if (!SaveGame)
	{
		return;
	}

	// 스트리밍-아웃 셀의 액터는 이미 LevelRemovedFromWorld 에서 기록됨.
	int32 CapturedCount = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		CapturedCount += CaptureActor(*SaveGame, *It) ? 1 : 0;
	}

	UE_LOG(LogWxSave, Log, TEXT("FlushSavableActors: %d개 캡처(기본값 그대로면 스킵), 누적 레코드 %d개"), CapturedCount, SaveGame->ActorRecords.Num());
}

void UWxSaveWorldSubsystem::FlushPlayerTransform(const FTransform* ResumeTransform)
{
	UWxSaveGame* SaveGame = GetActiveSaveGame();
	if (!SaveGame)
	{
		return;
	}

	if (ResumeTransform)
	{
		SaveGame->PlayerTransform = *ResumeTransform;
	}
	else
	{
		// 첫 플레이어 폰만 본다 — 스탠드얼론 싱글 전제이고 FlushPlayerStats 와 동일 대상이다.
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn)
		{
			return;
		}

		SaveGame->PlayerTransform = Pawn->GetActorTransform();
	}

	UE_LOG(LogWxSave, Log, TEXT("FlushPlayerTransform: 재개 지점 %s 캡처"), *SaveGame->PlayerTransform.GetLocation().ToString());
}

void UWxSaveWorldSubsystem::FlushPlayerStats()
{
	UWxSaveGame* SaveGame = GetActiveSaveGame();
	if (!SaveGame)
	{
		return;
	}

	// 첫 플레이어 폰만 본다 — 스탠드얼론 싱글 전제이고 FlushPlayerTransform 과 동일 대상이다.
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	SaveGame->PlayerStats.Reset();
	CapturePlayerStats(Pawn, SaveGame->PlayerStats);
	SaveGame->bHasPlayerStats = SaveGame->PlayerStats.Num() > 0;

	UE_LOG(LogWxSave, Log, TEXT("FlushPlayerStats: 어트리뷰트 %d개 캡처"), SaveGame->PlayerStats.Num());
}

void UWxSaveWorldSubsystem::CapturePlayerStats(AActor* PlayerActor, TMap<FName, float>& OutStats)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(PlayerActor);
	if (!ASC)
	{
		return;
	}

	// 구체 타입(WxCombatAttributeSet)을 참조하지 않고 리플렉션으로 순회해 WxSave 가 전투 도메인에 독립적이다.
	for (const UAttributeSet* Set : ASC->GetSpawnedAttributes())
	{
		if (!Set)
		{
			continue;
		}

		for (TFieldIterator<FStructProperty> It(Set->GetClass()); It; ++It)
		{
			if (It->Struct != FGameplayAttributeData::StaticStruct() || !It->HasAnyPropertyFlags(CPF_Net))
			{
				continue;
			}

			const FGameplayAttribute Attribute(*It);
			OutStats.Add(It->GetFName(), ASC->GetNumericAttributeBase(Attribute));
		}
	}
}

void UWxSaveWorldSubsystem::ApplyPlayerStats(AActor* PlayerActor, const TMap<FName, float>& InStats)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(PlayerActor);
	if (!ASC)
	{
		return;
	}

	// 1패스로 전량 적용하면 모든 Max 가 저장 값으로 확정된다(Max 는 다른 어트리뷰트에 의해 재조정되지 않으므로).
	// 2패스는 아직 저장 값과 다른 것(주로 잘못된 Max 로 클램프된 current)만 재적용해 정확한 Max 로 복원한다.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (const UAttributeSet* Set : ASC->GetSpawnedAttributes())
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

				// 이미 저장 값이면 재적용이 부르는 미세 드리프트를 막는다.
				if (Pass == 1 && ASC->GetNumericAttributeBase(Attribute) == *SavedValue)
				{
					continue;
				}

				ASC->SetNumericAttributeBase(Attribute, *SavedValue);
			}
		}
	}
}

bool UWxSaveWorldSubsystem::ShouldSave(const UObject* Object)
{
	const UObject* Archetype = Object->GetArchetype();
	if (!Archetype)
	{
		return true;
	}

	for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
	{
		if (!It->HasAnyPropertyFlags(CPF_SaveGame))
		{
			continue;
		}

		for (int32 Index = 0; Index < It->ArrayDim; ++Index)
		{
			if (!It->Identical_InContainer(Object, Archetype, Index))
			{
				return true;
			}
		}
	}

	return false;
}

bool UWxSaveWorldSubsystem::CaptureActor(UWxSaveGame& SaveGame, AActor* Actor)
{
	const IWxSavable* Savable = Cast<IWxSavable>(Actor);
	if (!Savable)
	{
		return false;
	}

	const FGuid ActorId = Savable->GetSaveId();
	if (!ActorId.IsValid())
	{
		UE_LOG(LogWxSave, Warning, TEXT("CaptureActor: '%s' 의 WxSaveId 가 유효하지 않아 저장에서 제외됨. 에디터에서 WxSaveId 부여 경로(PostLoad 등)가 동작했는지 확인하라."), *GetNameSafe(Actor));
		return false;
	}

	// 이 레코드의 블롭들(액터+컴포넌트)이 사용한 커스텀 버전의 합집합. archive 별로 컨테이너가 분리되므로 병합한다(같은 빌드라 GUID 당 버전이 같아 충돌 없음).
	FCustomVersionContainer UsedCustomVersions;
	FWxActorRecord Record;

	if (ShouldSave(Actor))
	{
		FMemoryWriter MemWriter(Record.ByteData, true);
		FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
		Ar.ArIsSaveGame = true;
		Actor->Serialize(Ar);
		UsedCustomVersions = MemWriter.GetCustomVersions();
	}

	// 컴포넌트의 UPROPERTY(SaveGame) 필드는 Actor::Serialize 가 자동으로 끌고 가지 않으므로 컴포넌트 FName 으로 별도 캡처.
	for (UActorComponent* Component : Actor->GetComponents())
	{
		if (!Component || !ShouldSave(Component))
		{
			continue;
		}

		FWxComponentRecord& ComponentRecord = Record.ComponentData.Add(Component->GetFName());

		FMemoryWriter MemWriter(ComponentRecord.ByteData, true);
		FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
		Ar.ArIsSaveGame = true;
		Component->Serialize(Ar);

		for (const FCustomVersion& Version : MemWriter.GetCustomVersions().GetAllVersions())
		{
			UsedCustomVersions.SetVersion(Version.Key, Version.Version, Version.GetFriendlyName());
		}
	}

	// 담을 것이 하나도 없다 — 옛 레코드까지 걷어 기본값으로 되돌아온 액터가 옛 상태로 복원되지 않게 한다.
	if (Record.ByteData.IsEmpty() && Record.ComponentData.IsEmpty())
	{
		SaveGame.ActorRecords.Remove(ActorId);
		return false;
	}

	Record.Transform = Actor->GetActorTransform();

	{
		FMemoryWriter HeaderWriter(Record.VersionHeader, true);
		FPackageFileVersion UEVersion = GPackageFileUEVersion;
		HeaderWriter << UEVersion;
		UsedCustomVersions.Serialize(HeaderWriter);
	}

	SaveGame.ActorRecords.Add(ActorId, MoveTemp(Record));
	return true;
}

bool UWxSaveWorldSubsystem::RestoreActor(const UWxSaveGame& SaveGame, AActor* Actor, bool* bOutIsSavable)
{
	IWxSavable* Savable = Cast<IWxSavable>(Actor);
	if (bOutIsSavable)
	{
		*bOutIsSavable = Savable != nullptr;
	}

	if (!Savable)
	{
		return false;
	}

	const FGuid ActorId = Savable->GetSaveId();
	if (!ActorId.IsValid())
	{
		UE_LOG(LogWxSave, Warning, TEXT("RestoreActor: '%s' 의 WxSaveId 가 유효하지 않아 복원할 수 없음."), *GetNameSafe(Actor));
		return false;
	}

	const FWxActorRecord* Record = SaveGame.ActorRecords.Find(ActorId);
	if (!Record)
	{
		return false;
	}

	Actor->SetActorTransform(Record->Transform);

	// 헤더가 없는 구버전 레코드는 현재 빌드 버전으로 읽는다.
	const bool bHasVersionHeader = Record->VersionHeader.Num() > 0;
	FPackageFileVersion SavedUEVersion = GPackageFileUEVersion;
	FCustomVersionContainer SavedCustomVersions;
	if (bHasVersionHeader)
	{
		FMemoryReader HeaderReader(Record->VersionHeader, true);
		HeaderReader << SavedUEVersion;
		SavedCustomVersions.Serialize(HeaderReader);
	}

	// 액터 본체는 기본값과 다른 것이 있을 때만 담기므로, 컴포넌트만 바뀐 레코드에선 비어 있다.
	if (!Record->ByteData.IsEmpty())
	{
		FMemoryReader MemReader(Record->ByteData, true);
		// 맨 FMemoryReader 는 커스텀 버전을 현재 빌드 값으로 리셋하므로, 역직렬화 전에 저장 시점 버전을 적용한다.
		if (bHasVersionHeader)
		{
			MemReader.SetUEVer(SavedUEVersion);
			MemReader.SetCustomVersions(SavedCustomVersions);
		}
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
		if (bHasVersionHeader)
		{
			MemReader.SetUEVer(SavedUEVersion);
			MemReader.SetCustomVersions(SavedCustomVersions);
		}
		FObjectAndNameAsStringProxyArchive Ar(MemReader, false);
		Ar.ArIsSaveGame = true;
		Component->Serialize(Ar);
	}

	Savable->OnSaveRestored();

	UE_LOG(LogWxSave, Verbose, TEXT("RestoreActor: '%s' (%s) 복원"), *GetNameSafe(Actor), *ActorId.ToString());
	return true;
}

void UWxSaveWorldSubsystem::HandleWorldInitializedActors(const UWorld::FActorsInitializedParams& Params)
{
	if (Params.World != GetWorld())
	{
		return;
	}

	const UWxSaveGame* SaveGame = GetActiveSaveGame();
	if (!SaveGame)
	{
		return;
	}

	// 신규 세션이면 레코드가 비어있어 noop.
	int32 SavableCount = 0;
	int32 RestoredCount = 0;
	for (TActorIterator<AActor> It(Params.World); It; ++It)
	{
		bool bIsSavable = false;
		RestoredCount += RestoreActor(*SaveGame, *It, &bIsSavable) ? 1 : 0;
		SavableCount += bIsSavable ? 1 : 0;
	}

	UE_LOG(LogWxSave, Log, TEXT("월드 초기화 복원: IWxSavable %d개 중 %d개에 슬롯 적용 (슬롯 레코드 %d개)"),
		SavableCount, RestoredCount, SaveGame->ActorRecords.Num());
}

void UWxSaveWorldSubsystem::HandleLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	if (!Level || World != GetWorld())
	{
		return;
	}

	const UWxSaveGame* SaveGame = GetActiveSaveGame();
	if (!SaveGame)
	{
		return;
	}

	int32 RestoredCount = 0;
	for (AActor* Actor : Level->Actors)
	{
		RestoredCount += RestoreActor(*SaveGame, Actor) ? 1 : 0;
	}

	UE_LOG(LogWxSave, Verbose, TEXT("스트리밍-인 복원: 레벨 '%s' — %d개 복원"), *Level->GetOutermost()->GetName(), RestoredCount);
}

void UWxSaveWorldSubsystem::HandleLevelRemovedFromWorld(ULevel* Level, UWorld* World)
{
	if (!Level || World != GetWorld())
	{
		return;
	}

	// 로드 직후 트래블 중이면 구 월드의 라이브 상태가 방금 로드한 세이브를 덮어쓰므로 캡처하지 않는다.
	const UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	if (!GameSubsystem || GameSubsystem->IsTravelingFromSaveFile())
	{
		return;
	}

	UWxSaveGame* SaveGame = GetActiveSaveGame();
	if (!SaveGame)
	{
		return;
	}

	int32 CapturedCount = 0;
	for (AActor* Actor : Level->Actors)
	{
		CapturedCount += CaptureActor(*SaveGame, Actor) ? 1 : 0;
	}

	UE_LOG(LogWxSave, Verbose, TEXT("스트리밍-아웃 캡처: 레벨 '%s' — %d개"), *Level->GetOutermost()->GetName(), CapturedCount);
}

void UWxSaveWorldSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
	if (World != GetWorld())
	{
		return;
	}

	const UWxSaveGameSubsystem* GameSubsystem = GetGameSubsystem();
	if (!GameSubsystem)
	{
		return;
	}

	// 로드 직후 트래블 중이면 구 월드의 라이브 상태가 방금 로드한 세이브를 덮어쓰므로 캡처하지 않는다.
	if (GameSubsystem->IsTravelingFromSaveFile())
	{
		UE_LOG(LogWxSave, Verbose, TEXT("teardown 캡처 스킵: 로드 트래블 중"));
		return;
	}

	// teardown 은 스트리밍-아웃·EndPlay 보다 앞서 발화하므로 전체 상태를 담고, 이후 개별 스트리밍-아웃 재캡처는 동일 데이터라 무해하다.
	UE_LOG(LogWxSave, Log, TEXT("맵 이탈 메모리 플러시: '%s'"), *World->GetMapName());
	FlushSavableActors();
}
