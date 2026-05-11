// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WxSaveManager.generated.h"

/**
 * 세이브 시스템 진입점.
 * 슬롯 관리, 비동기 저장/로드, Preset 기반 직렬화 정책을 게임 측에 제공한다.
 *
 * 사용 예 (예정):
 *  - SaveManager->SaveSlot("AutoSave")
 *  - SaveManager->LoadSlot("AutoSave")
 *  - SaveManager->GetCurrentSlotData<UWxSlotData>()
 */
UCLASS()
class WXSAVE_API UWxSaveManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
};
