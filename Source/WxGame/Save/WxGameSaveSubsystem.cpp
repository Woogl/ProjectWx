// Copyright Woogle. All Rights Reserved.

#include "Save/WxGameSaveSubsystem.h"

#include "Save/WxGameSlotData.h"

void UWxGameSaveSubsystem::SetLastCheckpoint(const FTransform& Transform)
{
	UWxGameSlotData* GameSlotData = Cast<UWxGameSlotData>(GetCurrentSlotData());
	if (!GameSlotData)
	{
		return;
	}

	GameSlotData->LastCheckpointTransform = Transform;
	GameSlotData->bHasCheckpoint = true;

	SaveSlot(DefaultSlotName);
}

bool UWxGameSaveSubsystem::HasValidCheckpoint() const
{
	const UWxGameSlotData* GameSlotData = Cast<UWxGameSlotData>(GetCurrentSlotData());
	return GameSlotData && GameSlotData->bHasCheckpoint;
}

const FTransform& UWxGameSaveSubsystem::GetLastCheckpoint() const
{
	const UWxGameSlotData* GameSlotData = Cast<UWxGameSlotData>(GetCurrentSlotData());
	if (!GameSlotData)
	{
		return FTransform::Identity;
	}
	return GameSlotData->LastCheckpointTransform;
}

void UWxGameSaveSubsystem::OnGameWorldReady(UWorld* World)
{
	Super::OnGameWorldReady(World);

	if (!bRequestedInitialLoad)
	{
		bRequestedInitialLoad = true;
		AsyncLoadSlot(DefaultSlotName);
	}
}
