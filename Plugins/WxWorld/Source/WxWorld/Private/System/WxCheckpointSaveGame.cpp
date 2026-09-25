// Copyright Woogle. All Rights Reserved.

#include "System/WxCheckpointSaveGame.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

bool UWxCheckpointSaveGame::SaveCheckpoint(const UWorld* World, const FTransform& Transform)
{
	if (!World || !World->IsNetMode(NM_Standalone) || !Transform.IsValid())
	{
		return false;
	}
	UWxCheckpointSaveGame* Save = Cast<UWxCheckpointSaveGame>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
	if (!Save)
	{
		return false;
	}
	Save->LevelPackage = FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
	Save->RespawnTransform = FTransform(Transform.GetRotation(), Transform.GetLocation());
	return UGameplayStatics::SaveGameToSlot(Save, GetSlotName(), 0);
}

bool UWxCheckpointSaveGame::TryGetCheckpoint(const UWorld* World, FTransform& OutTransform)
{
	if (!World || !World->IsNetMode(NM_Standalone))
	{
		return false;
	}
	const UWxCheckpointSaveGame* Save = Cast<UWxCheckpointSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSlotName(), 0));
	if (!Save || Save->LevelPackage.IsNone() || !Save->RespawnTransform.IsValid()
		|| Save->LevelPackage != FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName())))
	{
		return false;
	}
	OutTransform = FTransform(Save->RespawnTransform.GetRotation(), Save->RespawnTransform.GetLocation());
	return true;
}

bool UWxCheckpointSaveGame::ResetCheckpoint(const UWorld* World)
{
	if (!World || !World->IsNetMode(NM_Standalone))
	{
		return false;
	}
	const FString SlotName = GetSlotName();
	return !UGameplayStatics::DoesSaveGameExist(SlotName, 0) || UGameplayStatics::DeleteGameInSlot(SlotName, 0);
}

FString UWxCheckpointSaveGame::GetSlotName()
{
	return TEXT("WxCheckpoint");
}
