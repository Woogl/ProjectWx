// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WxGameMode.generated.h"

/**
 * 기본 GameMode.
 *
 * WxSave 슬롯이 RestartFromSave 로 세션에 적용된 상태(`UWxSaveSubsystem::IsSaveApplied`)면
 * 모든 PC 의 Pawn 을 saved transform 으로 spawn (4 인 멀티에서도 동일 좌표에 모이고 SpawnCollisionHandling 이 분산).
 * 그 외엔 부모의 default 흐름 (PlayerStart 기반).
 */
UCLASS()
class AWxGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
};
