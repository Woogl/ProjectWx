// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WxSlotInfo.generated.h"

/**
 * 슬롯 메타데이터. 슬롯 목록 화면(로드/이어하기 UI)을 빠르게 그리기 위한 가벼운 헤더.
 * 본 데이터(UWxSlotData)와 분리하여 별도 슬롯에 저장한다.
 *
 * 보통 보유 항목: 슬롯 이름, 마지막 플레이 시각, 플레이타임, 레벨 명, 스크린샷 등.
 */
UCLASS(Abstract)
class WXSAVE_API UWxSlotInfo : public USaveGame
{
	GENERATED_BODY()
};
