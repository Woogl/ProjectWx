// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxDamageFloaterInterface.generated.h"

UINTERFACE(MinimalAPI)
class UWxDamageFloaterInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 데미지 플로터 위젯이 구현해야 하는 인터페이스.
 * C++에서 위젯 생성 후 이 인터페이스를 통해 데미지 정보를 전달한다.
 */
class WXCOMBAT_API IWxDamageFloaterInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Damage Floater")
	void InitDamageInfo(float DamageAmount, bool bIsCritical);
};
