// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxSpawnable.generated.h"

DECLARE_MULTICAST_DELEGATE(FWxOnSpawnableKilled);

// 계약이 네이티브 델리게이트라 BP 만으로는 구현할 수 없다. BP 구현을 허용하면 픽커는 통과하지만 처치 통지가 오지 않는다.
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UWxSpawnable : public UInterface
{
	GENERATED_BODY()
};

/** 다른 주체가 스폰해 태어나는 액터의 계약. 스폰 주체를 아는 픽커가 MustImplement 로 이 계약을 강제한다. */
class WXCORE_API IWxSpawnable
{
	GENERATED_BODY()

public:
	/** 처치 판정은 스폰된 액터가 하고 이것으로 스폰 주체에 알린다. 스폰 주체의 처치 상태가 서버 권위이므로 서버에서 방송한다. */
	virtual FWxOnSpawnableKilled& GetOnKilledDelegate() = 0;
};
