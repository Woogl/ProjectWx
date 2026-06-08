// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WxSaveGame.generated.h"

/** 한 컴포넌트의 UPROPERTY(SaveGame) 직렬화 결과 바이트. UPROPERTY TMap 의 value 로 TArray 를 직접 받지 못해 wrapper. */
USTRUCT()
struct WXSAVE_API FWxComponentRecord
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<uint8> ByteData;
};

/** 한 액터의 상태 스냅샷. Transform + 액터 본체와 컴포넌트별 UPROPERTY(SaveGame) 직렬화 결과를 보관한다. */
USTRUCT()
struct WXSAVE_API FWxActorRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform Transform = FTransform::Identity;

	UPROPERTY()
	TArray<uint8> ByteData;

	UPROPERTY()
	TMap<FName, FWxComponentRecord> ComponentData;
};

/** WxSave 슬롯 데이터. 등록된 savable 액터들의 상태 맵 + 플레이어 부활 위치를 보관한다. */
UCLASS()
class WXSAVE_API UWxSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** WxSaveId -> 스냅샷. IWxSavableInterface::GetWxSaveId() 의 에디터-부여 영속 GUID 를 안정적 키로 사용한다(쿠킹 빌드 안전). */
	UPROPERTY()
	TMap<FGuid, FWxActorRecord> ActorRecords;

	/**
	 * 마지막 체크포인트 상호작용 시점의 플레이어 캐릭터 Transform. 사망 후 새 Pawn 스폰 위치로 사용된다.
	 * Identity 는 "미설정" sentinel — 신규 세션 + 체크포인트 미터치 상태와 같다 (월드 원점에 회전·스케일 디폴트인 체크포인트를 두지 않는다는 컨벤션).
	 */
	UPROPERTY()
	FTransform PlayerRespawnTransform = FTransform::Identity;
};
