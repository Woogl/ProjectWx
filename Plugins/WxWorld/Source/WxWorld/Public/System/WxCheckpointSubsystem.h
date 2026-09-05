// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WxCheckpointSubsystem.generated.h"

/** 맵 재시작 동안 유지하는 싱글플레이 부활 지점. 액터 참조는 보관하지 않는다. */
UCLASS()
class WXWORLD_API UWxCheckpointSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void RecordCheckpoint(const UWorld* World, const FTransform& Transform);
	bool TryGetCheckpoint(const UWorld* World, FTransform& OutTransform) const;
	void ResetCheckpoint();

private:
	FName LevelPackage;
	FTransform RespawnTransform = FTransform::Identity;
};
