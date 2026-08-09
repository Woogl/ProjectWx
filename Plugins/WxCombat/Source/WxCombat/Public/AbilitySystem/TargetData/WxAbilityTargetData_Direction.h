// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "WxAbilityTargetData_Direction.generated.h"

/** 클라이언트의 입력 방향을 서버로 전송하는 TargetData */
USTRUCT()
struct WXCOMBAT_API FWxAbilityTargetData_Direction : public FGameplayAbilityTargetData
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
