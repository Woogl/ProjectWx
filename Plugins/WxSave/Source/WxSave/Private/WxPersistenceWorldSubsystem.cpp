// Copyright Woogle. All Rights Reserved.

#include "WxPersistenceWorldSubsystem.h"

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
#include "WxPersistenceGameSubsystem.h"
#include "WxPersistenceSaveGame.h"
#include "WxSavable.h"
#include "WxSaveModule.h"

void UWxPersistenceWorldSubsystem::RequestSaveFlush(FOnSaveFlushComplete::FDelegate OnComplete)
{
	// Wx 엔 Mass 같은 페이즈 지연 작업이 없어 플러시가 전부 동기다(샘플은 여기서 Mass 스냅샷을 FrameEnd 로 지연).
	UWorld* World = GetWorld();

	// 맵 트래블 데이터는 명시적 저장에서만 캡처한다 — teardown 시엔 맵 전환을 일으킨 게임 코드가 다음 시작 지점의 소유자다.
	if (World && !World->bIsTearingDown)
	{
		FlushMapTravelData();
		FlushPlayerStats();
	}
	FlushSavableActors();

	OnComplete.ExecuteIfBound();
}

bool UWxPersistenceWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// 게임 월드만 저장 대상이고, 저장은 authority 전용이라 클라이언트 월드는 제외한다.
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && !World->IsNetMode(NM_Client);
}

void UWxPersistenceWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// FWorldDelegates 는 전역이라 모든 월드에서 발화한다 — 각 핸들러 선두에서 자기 월드만 필터한다(PIE 다중 인스턴스 격리 포함).
	WorldInitializedActorsHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UWxPersistenceWorldSubsystem::HandleWorldInitializedActors);
	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UWxPersistenceWorldSubsystem::HandleLevelAddedToWorld);
	LevelRemovedHandle = FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &UWxPersistenceWorldSubsystem::HandleLevelRemovedFromWorld);
	WorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UWxPersistenceWorldSubsystem::HandleWorldBeginTearDown);
}

void UWxPersistenceWorldSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitializedActorsHandle);
	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);
	FWorldDelegates::LevelRemovedFromWorld.Remove(LevelRemovedHandle);
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldBeginTearDownHandle);

	Super::Deinitialize();
}

void UWxPersistenceWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// 트래블 완료를 보고해 가드를 해제한다. 복원(OnWorldInitializedActors)과 구 월드 teardown 이 모두 끝난 뒤라 안전한 해제점이다.
	UGameInstance* GameInstance = InWorld.GetGameInstance();
	UWxPersistenceGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
	if (GameSubsystem && GameSubsystem->IsTravelingFromSaveFile())
	{
		GameSubsystem->ReportTravelFromSaveFileComplete(&InWorld);
	}
}

void UWxPersistenceWorldSubsystem::FlushMapTravelData()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWxPersistenceGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
	if (!GameSubsystem)
	{
		UE_LOG(LogWxSave, Warning, TEXT("FlushMapTravelData: WxPersistenceGameSubsystem 을 찾을 수 없음 — 트래블 데이터 미갱신"));
		return;
	}

	// 현재 맵 경로를 캡처한다. 부활 위치는 체크포인트가 RespawnTransform(SaveGame 최상위)에 직접 세팅하므로 TravelData 는 맵만 담는다.
	FWxPersistenceTravelData TravelData;
	TravelData.Map = FSoftObjectPath(UWxPersistenceGameSubsystem::GetStableMapPackageName(World).ToString());
	GameSubsystem->SetPersistenceTravelData(MoveTemp(TravelData));
}

void UWxPersistenceWorldSubsystem::FlushSavableActors()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWxPersistenceGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
	UWxPersistenceSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("FlushSavableActors: 활성 SaveGame 없음 — 캡처 중단"));
		return;
	}

	// 현재 월드의 IWxSavable 액터를 캡처. 스트리밍-아웃 셀의 액터는 이미 LevelRemovedFromWorld 에서 기록됨.
	int32 CapturedCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->Implements<UWxSavable>())
		{
			CaptureActor(*SaveGame, Actor);
			++CapturedCount;
		}
	}

	UE_LOG(LogWxSave, Log, TEXT("FlushSavableActors: IWxSavable %d개 캡처, 누적 레코드 %d개"), CapturedCount, SaveGame->ActorRecords.Num());
}

void UWxPersistenceWorldSubsystem::FlushPlayerStats()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWxPersistenceGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
	UWxPersistenceSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		return;
	}

	// 첫 플레이어 폰의 어트리뷰트를 캡처한다(스탠드얼론 싱글 전제 — FlushMapTravelData 와 동일 대상). 폰 부재 시 이전 캡처를 보존한다.
	const APlayerController* PC = World->GetFirstPlayerController();
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

void UWxPersistenceWorldSubsystem::CapturePlayerStats(AActor* PlayerActor, TMap<FName, float>& OutStats)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(PlayerActor);
	if (!ASC)
	{
		return;
	}

	// ASC 에 붙은 모든 AttributeSet 을 리플렉션으로 순회한다. 구체 타입(WxCombatAttributeSet)을 참조하지 않아 WxSave 가 전투 도메인에 독립적이다.
	for (const UAttributeSet* Set : ASC->GetSpawnedAttributes())
	{
		if (!Set)
		{
			continue;
		}

		for (TFieldIterator<FStructProperty> It(Set->GetClass()); It; ++It)
		{
			// 복제되는 어트리뷰트의 base 값만 담는다(CPF_Net 이 비복제 메타를 자동 제외).
			if (It->Struct != FGameplayAttributeData::StaticStruct() || !It->HasAnyPropertyFlags(CPF_Net))
			{
				continue;
			}

			const FGameplayAttribute Attribute(*It);
			OutStats.Add(It->GetFName(), ASC->GetNumericAttributeBase(Attribute));
		}
	}
}

void UWxPersistenceWorldSubsystem::ApplyPlayerStats(AActor* PlayerActor, const TMap<FName, float>& InStats)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(PlayerActor);
	if (!ASC)
	{
		return;
	}

	// Max 계열을 먼저, 나머지를 나중에 2패스로 세팅한다: current(HP 등)는 PreAttributeChange 에서 현재 Max 로 클램프되고
	// Max 변경은 PostAttributeChange 에서 current 를 비율 재조정하므로, Max 를 저장 값으로 먼저 세팅해야 이후 current 세팅이 정확히 복원된다.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		const bool bMaxPass = (Pass == 0);
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

				// "Max" 접두 어트리뷰트는 1패스, 나머지는 2패스에서만 적용한다.
				if (It->GetName().StartsWith(TEXT("Max")) != bMaxPass)
				{
					continue;
				}

				const FGameplayAttribute Attribute(*It);
				ASC->SetNumericAttributeBase(Attribute, *SavedValue);
			}
		}
	}
}

void UWxPersistenceWorldSubsystem::CaptureActor(UWxPersistenceSaveGame& SaveGame, AActor* Actor)
{
	const IWxSavable* Savable = Cast<IWxSavable>(Actor);
	if (!Savable)
	{
		return;
	}

	const FGuid ActorId = Savable->GetSaveId();
	if (!ActorId.IsValid())
	{
		UE_LOG(LogWxSave, Warning, TEXT("CaptureActor: '%s' 의 WxSaveId 가 유효하지 않아 저장에서 제외됨. 에디터에서 WxSaveId 부여 경로(PostLoad 등)가 동작했는지 확인하라."), *GetNameSafe(Actor));
		return;
	}

	FWxActorRecord& Record = SaveGame.ActorRecords.FindOrAdd(ActorId);
	Record.Transform = Actor->GetActorTransform();
	Record.ByteData.Reset();
	Record.ComponentData.Reset();

	// 이 레코드의 블롭들(액터+컴포넌트)이 사용한 커스텀 버전의 합집합. archive 별로 컨테이너가 분리되므로 병합한다(같은 빌드라 GUID 당 버전이 같아 충돌 없음).
	FCustomVersionContainer UsedCustomVersions;

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

		for (const FCustomVersion& Version : MemWriter.GetCustomVersions().GetAllVersions())
		{
			UsedCustomVersions.SetVersion(Version.Key, Version.Version, Version.GetFriendlyName());
		}
	}

	// 복원 시 맨 FMemoryReader 의 버전 리셋(현재 빌드 값으로 초기화) 함정을 막기 위해 [FPackageFileVersion][FCustomVersionContainer] 헤더를 별도 블롭으로 기록한다.
	Record.VersionHeader.Reset();
	FMemoryWriter HeaderWriter(Record.VersionHeader, true);
	FPackageFileVersion UEVersion = GPackageFileUEVersion;
	HeaderWriter << UEVersion;
	UsedCustomVersions.Serialize(HeaderWriter);
}

bool UWxPersistenceWorldSubsystem::RestoreActor(const UWxPersistenceSaveGame& SaveGame, AActor* Actor)
{
	IWxSavable* Savable = Cast<IWxSavable>(Actor);
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
		// 일치 레코드 없음 — 신규 세션/미저장 액터의 정상 경로.
		return false;
	}

	Actor->SetActorTransform(Record->Transform);

	// 레코드의 버전 헤더를 1회 파싱한다. 구버전 레코드(헤더 없음)는 기존과 동일하게 현재 빌드 버전으로 읽는다.
	const bool bHasVersionHeader = Record->VersionHeader.Num() > 0;
	FPackageFileVersion SavedUEVersion = GPackageFileUEVersion;
	FCustomVersionContainer SavedCustomVersions;
	if (bHasVersionHeader)
	{
		FMemoryReader HeaderReader(Record->VersionHeader, true);
		HeaderReader << SavedUEVersion;
		SavedCustomVersions.Serialize(HeaderReader);
	}

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

	Savable->OnWxSaveRestored();

	UE_LOG(LogWxSave, Verbose, TEXT("RestoreActor: '%s' (%s) 복원"), *GetNameSafe(Actor), *ActorId.ToString());
	return true;
}

void UWxPersistenceWorldSubsystem::HandleWorldInitializedActors(const UWorld::FActorsInitializedParams& Params)
{
	if (Params.World != GetWorld())
	{
		return;
	}

	UGameInstance* GameInstance = Params.World->GetGameInstance();
	UWxPersistenceGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
	const UWxPersistenceSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		return;
	}

	// 영구 레벨 + 초기 WP 셀 액터에 메모리 레코드 자동 복원. 신규 세션이면 레코드가 비어있어 noop.
	int32 SavableCount = 0;
	int32 RestoredCount = 0;
	for (TActorIterator<AActor> It(Params.World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->Implements<UWxSavable>())
		{
			++SavableCount;
			RestoredCount += RestoreActor(*SaveGame, Actor) ? 1 : 0;
		}
	}

	UE_LOG(LogWxSave, Log, TEXT("월드 초기화 복원: IWxSavable %d개 중 %d개에 슬롯 적용 (슬롯 레코드 %d개)"),
		SavableCount, RestoredCount, SaveGame->ActorRecords.Num());
}

void UWxPersistenceWorldSubsystem::HandleLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	if (!Level || World != GetWorld())
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	UWxPersistenceGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
	const UWxPersistenceSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		return;
	}

	// 스트리밍-인 셀(World Partition cell 포함) 액터에 메모리 레코드 자동 복원.
	int32 RestoredCount = 0;
	for (AActor* Actor : Level->Actors)
	{
		if (Actor && Actor->Implements<UWxSavable>())
		{
			RestoredCount += RestoreActor(*SaveGame, Actor) ? 1 : 0;
		}
	}

	UE_LOG(LogWxSave, Verbose, TEXT("스트리밍-인 복원: 레벨 '%s' — %d개 복원"), *Level->GetOutermost()->GetName(), RestoredCount);
}

void UWxPersistenceWorldSubsystem::HandleLevelRemovedFromWorld(ULevel* Level, UWorld* World)
{
	if (!Level || World != GetWorld())
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	UWxPersistenceGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
	UWxPersistenceSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		return;
	}

	// 로드 직후 트래블 중이면 구 월드의 라이브 상태가 방금 로드한 세이브를 덮어쓰므로 캡처하지 않는다.
	if (GameSubsystem->IsTravelingFromSaveFile())
	{
		return;
	}

	// 스트리밍-아웃 직전 자동 캡처: 셀 왕복으로 인한 상태 손실 방지.
	int32 CapturedCount = 0;
	for (AActor* Actor : Level->Actors)
	{
		if (Actor && Actor->Implements<UWxSavable>())
		{
			CaptureActor(*SaveGame, Actor);
			++CapturedCount;
		}
	}

	UE_LOG(LogWxSave, Verbose, TEXT("스트리밍-아웃 캡처: 레벨 '%s' — IWxSavable %d개"), *Level->GetOutermost()->GetName(), CapturedCount);
}

void UWxPersistenceWorldSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
	if (World != GetWorld())
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	UWxPersistenceGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>() : nullptr;
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

	// 맵 이탈 시 IWxSavable 전체를 메모리에 플러시한다(디스크 기록 없음 — 같은 세션 맵 왕복 상태 유지).
	// teardown 은 스트리밍-아웃·EndPlay 보다 앞서 발화하므로 전체 상태를 담고, 이후 개별 스트리밍-아웃 재캡처는 동일 데이터라 무해하다.
	// 트래블 데이터/PlayerStartTag 는 스탬프하지 않는다(디스크 영속은 명시적 SaveToFile 만, 다음 시작 지점은 맵 전환을 일으킨 게임 코드 소유).
	UE_LOG(LogWxSave, Log, TEXT("맵 이탈 메모리 플러시: '%s'"), *World->GetMapName());
	FlushSavableActors();
}
