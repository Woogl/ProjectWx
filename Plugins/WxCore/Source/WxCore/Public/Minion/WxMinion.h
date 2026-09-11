// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxMinion.generated.h"

/**
 * 소환될 수 있는 액터의 계약. 소환물 종류가 자기 상한을 선언하므로 소환 몽타주가 여럿이어도 값은 한 곳에 있다.
 * 구현체는 GenericTeamAgentInterface 도 함께 구현해야 한다 — 소환물은 주인의 팀을 물려받는다.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UWxMinion : public UInterface
{
	GENERATED_BODY()
};

class WXCORE_API IWxMinion
{
	GENERATED_BODY()

public:
	/** 0이면 개수 제한이 없다. 양수이면 해당 개수를 넘길 때 주인의 가장 오래된 소환물부터 파괴한다. 음수는 0으로 보정한다. */
	UFUNCTION(BlueprintNativeEvent, Category = "Wx|Minion")
	int32 GetMaxCountPerMaster() const;
	virtual int32 GetMaxCountPerMaster_Implementation() const;

	/** true이면 감지되거나 공격을 가해도 AI의 어그로 대상이 되지 않는다. 미구현 BP는 false이므로 일반 미니언은 어그로를 끈다. */
	UFUNCTION(BlueprintNativeEvent, Category = "Wx|Minion")
	bool IsAggroIgnored() const;
	virtual bool IsAggroIgnored_Implementation() const;
};
