// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WxSlotData.generated.h"

/**
 * 슬롯의 실제 게임 상태 베이스 클래스. 인벤토리, 어트리뷰트, 월드 진행도 등 게임 특화 필드는
 * 본 클래스를 상속한 프로젝트 측 서브클래스에 정의한다. Project Settings → Plugins → WxSave 에서 SlotDataClass 로 등록.
 *
 * 슬롯 목록을 그릴 때는 본 클래스를 로드하지 않고 UWxSlotInfo 만 읽는다.
 *
 * 패치 호환성을 위해 SaveVersion 멤버를 가지며, 서브클래스가 마이그레이션 분기를 추가할 수 있다.
 */
UCLASS()
class WXSAVE_API UWxSlotData : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 SaveVersion = 1;
};
