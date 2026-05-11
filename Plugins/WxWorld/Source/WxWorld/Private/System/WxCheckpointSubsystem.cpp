// Copyright Woogle. All Rights Reserved.

#include "System/WxCheckpointSubsystem.h"

void UWxCheckpointSubsystem::SetLastCheckpoint(const FTransform& InTransform)
{
	LastCheckpointTransform = InTransform;
	bHasValidCheckpoint = true;
	OnLastCheckpointChanged.Broadcast(LastCheckpointTransform);
}

const FTransform& UWxCheckpointSubsystem::GetLastCheckpoint() const
{
	return LastCheckpointTransform;
}

bool UWxCheckpointSubsystem::HasValidCheckpoint() const
{
	return bHasValidCheckpoint;
}
