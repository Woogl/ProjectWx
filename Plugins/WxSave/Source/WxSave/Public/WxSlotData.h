// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WxSlotData.generated.h"

/**
 * 슬롯의 실제 게임 상태. 부활 지점, 인벤토리, 어트리뷰트, 월드 진행도 등이 누적된다.
 * 슬롯 목록을 그릴 때는 본 클래스를 로드하지 않고 UWxSlotInfo 만 읽는다.
 *
 * 패치 호환성을 위해 SaveVersion 멤버를 가지며 마이그레이션 분기가 들어간다.
 */
UCLASS()
class WXSAVE_API UWxSlotData : public USaveGame
{
	GENERATED_BODY()
};
