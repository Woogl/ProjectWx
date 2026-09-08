// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxMinion.generated.h"

/**
 * 소환될 수 있는 액터의 계약. 소환물 종류가 자기 상한을 선언하므로 소환 몽타주가 여럿이어도 값은 한 곳에 있다.
 * 구현체는 GenericTeamAgentInterface 도 함께 구현해야 한다 — 소환물은 주인의 팀을 물려받는다.
 */
UINTERFACE(MinimalAPI, NotBlueprintable, meta = (CannotImplementInterfaceInBlueprint))
class UWxMinion : public UInterface
{
	GENERATED_BODY()
};

class WXCOMBAT_API IWxMinion
{
	GENERATED_BODY()

public:
	/** 주인이 이 소환물을 동시에 몇 마리까지 유지하는가. 넘치면 주인의 가장 오래된 소환물부터 파괴된다. */
	virtual int32 GetMaxCountPerMaster() const = 0;
};
