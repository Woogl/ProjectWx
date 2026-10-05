// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "WxAbilityTargetData_Direction.generated.h"

/** 서버·클라이언트 사이에서 방향 벡터를 나르는 TargetData — 클라이언트의 입력 방향을 서버로, 서버가 정한 피격 방향을 Event.Hit에 실어 보낸다. */
USTRUCT()
struct WXGAME_API FWxAbilityTargetData_Direction : public FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector Direction = FVector::ZeroVector;

	virtual UScriptStruct* GetScriptStruct() const override;
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FWxAbilityTargetData_Direction> : public TStructOpsTypeTraitsBase2<FWxAbilityTargetData_Direction>
{
	enum
	{
		WithNetSerializer = true,
	};
};
