// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxSpawnable.generated.h"

UINTERFACE(MinimalAPI)
class UWxSpawnable : public UInterface
{
	GENERATED_BODY()
};

/** 다른 주체가 스폰해 태어나는 액터의 계약. 스폰 주체를 아는 픽커가 MustImplement 로 이 계약을 강제한다. */
class WXCORE_API IWxSpawnable
{
	GENERATED_BODY()

public:
	// Deferred Spawn 의 FinishSpawning 이전에 불리므로, 여기서 세팅한 값은 AIController 빙의/BeginPlay 에서 사용할 수 있다.
	virtual void OnSpawnedBy(AActor* Spawner);
};
