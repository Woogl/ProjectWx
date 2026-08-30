// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UObject/SoftObjectPath.h"
#include "WxSaveGame.generated.h"

USTRUCT()
struct WXSAVE_API FWxInstancedActorManagerState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<uint8> Data;
};

USTRUCT()
struct WXSAVE_API FWxStreamingLevelPersistenceEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FName, FWxInstancedActorManagerState> InstancedActorManagerDeltas;
};

USTRUCT()
struct WXSAVE_API FWxMassFragmentLayout
{
	GENERATED_BODY()

	UPROPERTY()
	FSoftObjectPath Type;

	UPROPERTY()
	int32 SizeInBytes = 0;
};

USTRUCT()
struct WXSAVE_API FWxMassEntityConfigGroupSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FWxMassFragmentLayout> FragmentLayout;

	UPROPERTY()
	int32 EntityCount = 0;

	UPROPERTY()
	TArray<uint8> Data;

	UPROPERTY()
	FSoftObjectPath SourceConfigAsset;
};

USTRUCT()
struct WXSAVE_API FWxWorldPersistenceEntry
{
	GENERATED_BODY()

	/** ULevelStreamingPersistenceManager가 만든 맵 전체 속성 상태. */
	UPROPERTY()
	TArray<uint8> StreamingLevelData;

	/** PIE 접두사에 영향받지 않는 스트리밍 레벨 패키지 이름별 Instanced Actors 상태. */
	UPROPERTY()
	TMap<FName, FWxStreamingLevelPersistenceEntry> SavedStatePerStreamingLevel;

	/** 영속화 Trait을 사용한 일반 Mass 엔티티의 EntityConfig별 스냅샷. */
	UPROPERTY()
	TArray<FWxMassEntityConfigGroupSnapshot> MassEntitySnapshots;
};

USTRUCT()
struct WXSAVE_API FWxSaveTravelData
{
	GENERATED_BODY()

	UPROPERTY()
	FSoftObjectPath Map;

	UPROPERTY()
	bool bHasPawnTransform = false;

	UPROPERTY()
	FTransform PawnTransform = FTransform::Identity;

	UPROPERTY()
	bool bHasControlRotation = false;

	UPROPERTY()
	FRotator ControlRotation = FRotator::ZeroRotator;
};

UCLASS()
class WXSAVE_API UWxSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 새 스키마 생성 경로에서만 기록한다. 구 슬롯의 누락 기본값 0과 구분한다. */
	UPROPERTY()
	int32 SaveFormatVersion = 0;

	UPROPERTY()
	FString SlotName;

	UPROPERTY()
	int32 UserIndex = 0;

	UPROPERTY()
	FWxSaveTravelData TravelData;

	UPROPERTY()
	bool bHasPlayerStats = false;

	UPROPERTY()
	TMap<FName, float> PlayerStats;

	/** 안정화된 맵 패키지 이름별 월드 영속 상태. */
	UPROPERTY()
	TMap<FName, FWxWorldPersistenceEntry> SavedStatePerMap;
};
