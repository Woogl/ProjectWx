// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WxPlayerSave.generated.h"

/**
 * 한 컴포넌트의 SaveGame 필드 직렬화 결과 바이트.
 * UPROPERTY TMap 이 TArray 를 직접 value 로 받지 못해 wrapper 가 필요하다.
 */
USTRUCT()
struct WXSAVE_API FWxComponentRecord
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<uint8> ByteData;
};

/**
 * 한 액터의 상태 스냅샷.
 *
 * Transform 과 액터 본체 + 컴포넌트별 UPROPERTY(SaveGame) 직렬화 결과를 보관한다.
 * UWxSaveSubsystem 내부 포맷.
 */
USTRUCT()
struct WXSAVE_API FWxActorRecord
{
	GENERATED_BODY()

	/** 저장 시점 액터의 월드 Transform. 이동형 액터(엘리베이터 등) 의 위치 복원에 사용. */
	UPROPERTY()
	FTransform Transform = FTransform::Identity;

	/** 액터 본체의 UPROPERTY(SaveGame) 필드 직렬화 결과. */
	UPROPERTY()
	TArray<uint8> ByteData;

	/** 컴포넌트별 UPROPERTY(SaveGame) 직렬화 결과. 컴포넌트 FName 을 키로 매칭하여 복원한다. */
	UPROPERTY()
	TMap<FName, FWxComponentRecord> ComponentData;
};

/**
 * 게임 슬롯 SaveGame. UWxSaveSubsystem 이 슬롯 IO 시 직접 직렬화한다.
 *
 * 단일 슬롯에 플레이어 위치 + 월드의 모든 IWxSavable 액터 상태를 보존한다.
 * 슬롯명/슬롯 인덱스는 UWxSaveDeveloperSettings 에서 조회한다.
 */
UCLASS()
class WXSAVE_API UWxPlayerSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** 마지막 체크포인트 시점 플레이어 캐릭터의 월드 Transform. */
	UPROPERTY()
	FTransform PlayerTransform = FTransform::Identity;

	/** 유효한 플레이어 위치가 저장되어 있는지 여부. */
	UPROPERTY()
	bool bHasSavedLocation = false;

	/** 저장 시점의 맵 이름. RestartFromSave 가 이 맵을 reload 한다. */
	UPROPERTY()
	FString MapName;

	/** 액터 GUID -> 스냅샷. 레벨 배치 액터의 ActorGuid 만 안정적 키로 사용된다. */
	UPROPERTY()
	TMap<FGuid, FWxActorRecord> ActorRecords;
};
