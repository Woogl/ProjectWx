// Copyright Woogle. All Rights Reserved.

#include "System/WxCheckpointSubsystem.h"

#include "Engine/World.h"

void UWxCheckpointSubsystem::RecordCheckpoint(const UWorld* World, const FTransform& Transform)
{
	if (!World || !World->IsNetMode(NM_Standalone) || Transform.ContainsNaN())
	{
		return;
	}
	LevelPackage = FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
	RespawnTransform = FTransform(Transform.GetRotation(), Transform.GetLocation());
}

bool UWxCheckpointSubsystem::TryGetCheckpoint(const UWorld* World, FTransform& OutTransform) const
{
	if (!World || !World->IsNetMode(NM_Standalone) || LevelPackage.IsNone()
		|| LevelPackage != FName(*UWorld::RemovePIEPrefix(World->GetOutermost()->GetName())))
	{
		return false;
	}
	OutTransform = RespawnTransform;
	return true;
}

void UWxCheckpointSubsystem::ResetCheckpoint()
{
	LevelPackage = NAME_None;
	RespawnTransform = FTransform::Identity;
}
