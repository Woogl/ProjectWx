// Copyright Woogle. All Rights Reserved.

#include "WxSaveSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectHash.h"
#include "WxSaveDeveloperSettings.h"
#include "WxSlotData.h"

const FString UWxSaveSubsystem::DefaultSlotName(TEXT("AutoSave"));

void UWxSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UClass* SlotClass = ResolveSlotDataClass();
	CurrentSlotData = NewObject<UWxSlotData>(this, SlotClass);

	PostWorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UWxSaveSubsystem::HandlePostWorldInit);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UWxSaveSubsystem::HandleWorldCleanup);
}

void UWxSaveSubsystem::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitHandle);
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);

	Super::Deinitialize();
}

bool UWxSaveSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// 파생 클래스가 존재하면 베이스는 생성하지 않는다. 파생이 대신 생성됨.
	TArray<UClass*> Subclasses;
	GetDerivedClasses(GetClass(), Subclasses, false);
	return Subclasses.Num() == 0;
}

void UWxSaveSubsystem::SaveSlot(const FString& SlotName)
{
	if (!CurrentSlotData || SlotName.IsEmpty())
	{
		return;
	}

	OnPreSerialize(CurrentSlotData);
	UGameplayStatics::AsyncSaveGameToSlot(CurrentSlotData, SlotName, 0);
}

void UWxSaveSubsystem::AsyncLoadSlot(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		return;
	}

	FAsyncLoadGameFromSlotDelegate LoadedDelegate;
	LoadedDelegate.BindUObject(this, &UWxSaveSubsystem::HandleAsyncLoadCompleted);
	UGameplayStatics::AsyncLoadGameFromSlot(SlotName, 0, LoadedDelegate);
}

UWxSlotData* UWxSaveSubsystem::GetCurrentSlotData() const
{
	return CurrentSlotData;
}

void UWxSaveSubsystem::HandleAsyncLoadCompleted(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGame)
{
	if (UWxSlotData* Loaded = Cast<UWxSlotData>(LoadedGame))
	{
		CurrentSlotData = Loaded;
		OnPostDeserialize(Loaded);
	}

	OnCurrentSlotLoaded.Broadcast(CurrentSlotData);
}

void UWxSaveSubsystem::HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues /*IVS*/)
{
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	OnGameWorldReady(World);
}

void UWxSaveSubsystem::HandleWorldCleanup(UWorld* World, bool /*bSessionEnded*/, bool /*bCleanupResources*/)
{
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	OnGameWorldCleanup(World);
}

UClass* UWxSaveSubsystem::ResolveSlotDataClass() const
{
	const UWxSaveDeveloperSettings* Settings = GetDefault<UWxSaveDeveloperSettings>();
	if (!Settings || !Settings->SlotDataClass)
	{
		return UWxSlotData::StaticClass();
	}

	return Settings->SlotDataClass.Get();
}
