// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxSlotData.h"
#include "WxGameSlotData.generated.h"

/**
 * 본 게임의 SlotData. WxSave 베이스에 게임 특화 필드를 추가한다.
 * Project Settings → WxSave → DefaultPreset 에 지정된 UWxSavePreset 의 SlotDataClass 로 등록.
 */
UCLASS()
class UWxGameSlotData : public UWxSlotData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FTransform LastCheckpointTransform = FTransform::Identity;

	UPROPERTY()
	bool bHasCheckpoint = false;
};
