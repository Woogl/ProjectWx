// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WxPersistableActorReference.h"
#include "WxPersistedAbilitySystemState.generated.h"

class UGameplayEffect;

/** opt-in GameplayEffect 하나를 다시 구성하는 데 필요한 최소 상태. */
USTRUCT()
struct FWxPersistedGameplayEffect
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftClassPtr<UGameplayEffect> EffectClass;

	UPROPERTY()
	FWxPersistableActorReference Instigator;

	/** -1은 무한 지속. */
	UPROPERTY()
	float RemainingDuration = -1.f;

	UPROPERTY()
	float Level = 1.f;

	UPROPERTY()
	int32 StackCount = 1;

	UPROPERTY()
	TMap<FName, float> SetByCallerNameMagnitudes;

	UPROPERTY()
	TMap<FGameplayTag, float> SetByCallerTagMagnitudes;
};

USTRUCT()
struct FWxPersistedAbilitySystemState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FWxPersistedGameplayEffect> GameplayEffects;
};
