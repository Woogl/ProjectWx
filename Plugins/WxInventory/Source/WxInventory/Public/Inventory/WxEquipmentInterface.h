// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "WxEquipmentInterface.generated.h"

class UWxItemDefinition;

UINTERFACE(MinimalAPI)
class UWxEquipmentInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 아이템 장착 처리 인터페이스.
 * WxInventory에서 게임 측(Character 등)으로 장착 요청을 플러그인 의존 없이 전달한다.
 */
class WXINVENTORY_API IWxEquipmentInterface
{
	GENERATED_BODY()

public:
	/** 권한: ItemDef의 Equipment Fragment에 따라 시각/부착을 반영 */
	virtual void EquipItem(const UWxItemDefinition* ItemDef) = 0;
};
