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

/** WxSave 슬롯 데이터. 등록된 savable 액터들의 상태 맵 + 부활/시작 PlayerStart 식별자를 보관한다. */
UCLASS()
class WXSAVE_API UWxSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** WxSaveId -> 스냅샷. IWxSavable::GetWxSaveId() 의 에디터-부여 영속 GUID 를 안정적 키로 사용한다(쿠킹 빌드 안전). */
	UPROPERTY()
	TMap<FGuid, FWxActorRecord> ActorRecords;

	/**
	 * 마지막으로 시작/상호작용한 PlayerStart 의 PlayerStartTag. 레벨 시작 지점이자 사망 후 새 Pawn 스폰 지점으로 사용된다.
	 * (대표 사용처는 체크포인트 AWxCheckPoint=APlayerStart 지만, 일반 PlayerStart 도 동일하게 기록된다.)
	 * NAME_None 은 "미설정" sentinel — 신규 세션 + 미기록 상태와 같다(이때 기본 PlayerStart 로 폴백).
	 * 좌표가 아니라 식별자만 저장하므로 ChoosePlayerStart 가 엔진 FindPlayerStart(Tag) 로 실제 배치된 액터를 찾아 부활시킨다.
	 */
	UPROPERTY()
	FName PlayerStartTag = NAME_None;
};
