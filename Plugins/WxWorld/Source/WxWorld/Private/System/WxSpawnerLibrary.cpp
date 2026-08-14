// Copyright Woogle. All Rights Reserved.

#include "System/WxSpawnerLibrary.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Spawnable/WxSpawner.h"
#include "UniversalObjectLocator.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"

void UWxSpawnerLibrary::TryRespawnAll(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	// Respawn() 이 액터를 Destroy/Spawn 하므로, 순회 도중 월드 액터 배열이 바뀌지 않도록 먼저 모은 뒤 일괄 호출한다.
	// Manual 스포너는 외부 개별 트리거 전용이라 일괄 리스폰에서 제외한다.
	TArray<AWxSpawner*> Spawners;
	for (TActorIterator<AWxSpawner> It(World); It; ++It)
	{
		AWxSpawner* Spawner = *It;
		if (Spawner && Spawner->GetSpawnMode() != EWxSpawnerMode::Manual)
		{
			Spawners.Add(Spawner);
		}
	}

	for (AWxSpawner* Spawner : Spawners)
	{
		Spawner->Respawn();
	}
}

#if WITH_EDITOR
FString UWxSpawnerLibrary::GetSpawnerLocatorDisplayName(const FUniversalObjectLocator& Locator)
{
	if (Locator.IsEmpty())
	{
		return TEXT("none");
	}

	if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
	{
		return Actor->GetActorLabel();
	}

	const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
	const FActorLocatorFragment* Payload = nullptr;
	if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
	{
		const FString SubPath = Payload->Path.GetSubPathString();
		int32 DotIndex = INDEX_NONE;
		return SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath;
	}

	return TEXT("unresolved");
}

FText UWxSpawnerLibrary::GetSpawnerLocatorsText(const TArray<FUniversalObjectLocator>& Spawners)
{
	if (Spawners.IsEmpty())
	{
		return INVTEXT("none");
	}

	constexpr int32 MaxNames = 3;
	TArray<FString> Names;
	for (int32 Index = 0; Index < Spawners.Num() && Index < MaxNames; ++Index)
	{
		Names.Add(GetSpawnerLocatorDisplayName(Spawners[Index]));
	}

	FString Joined = FString::Join(Names, TEXT(", "));
	if (Spawners.Num() > MaxNames)
	{
		Joined += FString::Printf(TEXT(" +%d"), Spawners.Num() - MaxNames);
	}
	return FText::FromString(Joined);
}
#endif
