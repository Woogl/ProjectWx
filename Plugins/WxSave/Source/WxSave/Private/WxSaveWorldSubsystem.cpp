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

	// 맵 트래블 데이터와 플레이어 스냅샷은 명시적 저장에서만 캡처한다 — teardown 시엔 맵 전환을 일으킨 게임 코드가 다음 시작 지점의 소유자다.
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

	// 게임 월드만 저장 대상이고, 저장은 authority 전용이라 클라이언트 월드는 제외한다.
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

	// 트래블 완료를 보고해 가드를 해제한다. 복원(OnWorldInitializedActors)과 구 월드 teardown 이 모두 끝난 뒤라 안전한 해제점이다.
	UGameInstance* GameInstance = InWorld.GetGameInstance();
	UWxSaveGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	if (GameSubsystem && GameSubsystem->IsTravelingFromSaveFile())
	{
		GameSubsystem->ReportTravelFromSaveFileComplete(&InWorld);
	}
}

IWxSavable* UWxSaveWorldSubsystem::FindSavable(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	// 액터가 직접 구현했으면 그것이 답이다(스포너 등 영속이 액터 고유 상태인 경우).
	if (IWxSavable* ActorImplementation = Cast<IWxSavable>(Actor))
	{
		return ActorImplementation;
	}

	// 아니면 컴포넌트가 계약을 든다(기믹). 액터 자체는 C++ 없이 순수 BP 일 수 있다.
	return Cast<IWxSavable>(Actor->FindComponentByInterface(UWxSavable::StaticClass()));
}

void UWxSaveWorldSubsystem::FlushMapTravelData()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWxSaveGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	if (!GameSubsystem)
	{
		UE_LOG(LogWxSave, Warning, TEXT("FlushMapTravelData: WxSaveGameSubsystem 을 찾을 수 없음 — 트래블 데이터 미갱신"));
		return;
	}

	// 현재 맵 경로를 캡처한다. 재개 지점은 FlushPlayerTransform 이 SaveGame 최상위에 담으므로 TravelData 는 맵만 담는다.
	FWxSaveTravelData TravelData;
	TravelData.Map = FSoftObjectPath(UWxSaveGameSubsystem::GetStableMapPackageName(World).ToString());
	GameSubsystem->SetTravelData(MoveTemp(TravelData));
}

void UWxSaveWorldSubsystem::FlushSavableActors()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWxSaveGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	UWxSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		UE_LOG(LogWxSave, Warning, TEXT("FlushSavableActors: 활성 SaveGame 없음 — 캡처 중단"));
		return;
	}

	// 현재 월드의 IWxSavable 액터를 캡처. 스트리밍-아웃 셀의 액터는 이미 LevelRemovedFromWorld 에서 기록됨.
	int32 CapturedCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		CapturedCount += CaptureActor(*SaveGame, *It) ? 1 : 0;
	}

	UE_LOG(LogWxSave, Log, TEXT("FlushSavableActors: IWxSavable %d개 캡처, 누적 레코드 %d개"), CapturedCount, SaveGame->ActorRecords.Num());
}

void UWxSaveWorldSubsystem::FlushPlayerTransform(const FTransform* ResumeTransform)
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWxSaveGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	UWxSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		return;
	}

	if (ResumeTransform)
	{
		// 저장 요청자가 재개 지점을 지정했다(체크포인트 오토세이브). 폰이 어디에 서 있었든 그 자리로 확정한다.
		SaveGame->PlayerTransform = *ResumeTransform;
	}
	else
	{
		// 첫 플레이어 폰의 위치를 재개 지점으로 캡처한다(스탠드얼론 싱글 전제 — FlushPlayerStats 와 동일 대상). 폰 부재 시 이전 캡처를 보존한다.
		const APlayerController* PC = World->GetFirstPlayerController();
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
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWxSaveGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	UWxSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
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

void UWxSaveWorldSubsystem::CapturePlayerStats(AActor* PlayerActor, TMap<FName, float>& OutStats)
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

void UWxSaveWorldSubsystem::ApplyPlayerStats(AActor* PlayerActor, const TMap<FName, float>& InStats)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(PlayerActor);
	if (!ASC)
	{
		return;
	}

	// 어트리뷰트 간 세팅 순서 의존(current 는 PreAttributeChange 에서 현재 Max 로 클램프되고, Max 변경은
	// PostAttributeChange 에서 current 를 비율 재조정)을 이름 규칙 없이 흡수한다. 1패스로 전량 적용하면 모든 Max 가
	// 저장 값으로 확정되고(Max 는 다른 어트리뷰트에 의해 재조정되지 않으므로), 2패스는 아직 저장 값과 다른 것(주로
	// 잘못된 Max 로 클램프된 current)만 재적용해 이제 정확한 Max 로 저장 값에 정확히 복원한다.
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

				// 이미 저장 값이면(주로 1패스에서 확정된 Max) 재적용을 건너뛴다. 불필요한 재조정에 의한 미세 드리프트를 막는다.
				if (Pass == 1 && ASC->GetNumericAttributeBase(Attribute) == *SavedValue)
				{
					continue;
				}

				ASC->SetNumericAttributeBase(Attribute, *SavedValue);
			}
		}
	}
}

bool UWxSaveWorldSubsystem::CaptureActor(UWxSaveGame& SaveGame, AActor* Actor)
{
	const IWxSavable* Savable = FindSavable(Actor);
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

	return true;
}

bool UWxSaveWorldSubsystem::RestoreActor(const UWxSaveGame& SaveGame, AActor* Actor)
{
	IWxSavable* Savable = FindSavable(Actor);
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

	UGameInstance* GameInstance = Params.World->GetGameInstance();
	UWxSaveGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	const UWxSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
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
		if (FindSavable(Actor))
		{
			++SavableCount;
			RestoredCount += RestoreActor(*SaveGame, Actor) ? 1 : 0;
		}
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

	UGameInstance* GameInstance = World->GetGameInstance();
	UWxSaveGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	const UWxSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
	if (!SaveGame)
	{
		return;
	}

	// 스트리밍-인 셀(World Partition cell 포함) 액터에 메모리 레코드 자동 복원.
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

	UGameInstance* GameInstance = World->GetGameInstance();
	UWxSaveGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	UWxSaveGame* SaveGame = GameSubsystem ? GameSubsystem->GetSaveGame() : nullptr;
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
		CapturedCount += CaptureActor(*SaveGame, Actor) ? 1 : 0;
	}

	UE_LOG(LogWxSave, Verbose, TEXT("스트리밍-아웃 캡처: 레벨 '%s' — IWxSavable %d개"), *Level->GetOutermost()->GetName(), CapturedCount);
}

void UWxSaveWorldSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
	if (World != GetWorld())
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	UWxSaveGameSubsystem* GameSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
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
