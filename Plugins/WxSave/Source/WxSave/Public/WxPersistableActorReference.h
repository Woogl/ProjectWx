// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "WxPersistableActorReference.generated.h"

class AActor;
class UWorld;

UENUM()
enum class EWxPersistableActorReferenceType : uint8
{
	None,
	LevelActor,
	PlayerPawn,
	PlayerController
};

/** 세션을 넘겨도 다시 찾을 수 있는 액터 식별자. 런타임 액터는 이전 세션의 레벨 경로·이름으로 찾는다. */
USTRUCT()
struct WXSAVE_API FWxPersistableActorReference
{
	GENERATED_BODY()

	bool Capture(const AActor* Actor);
	AActor* Resolve(UWorld* World) const;
	bool IsSet() const;

	UPROPERTY()
	EWxPersistableActorReferenceType Type = EWxPersistableActorReferenceType::None;

	UPROPERTY()
	FSoftObjectPath LevelPath;

	UPROPERTY()
	FName ActorName;

	UPROPERTY()
	int32 PlayerIndex = INDEX_NONE;
};
