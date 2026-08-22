// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxSpawnable.generated.h"

class AWxSpawner;

UINTERFACE(MinimalAPI)
class UWxSpawnable : public UInterface
{
	GENERATED_BODY()
};

class WXWORLD_API IWxSpawnable
{
	GENERATED_BODY()

public:
	// Deferred Spawn 의 FinishSpawning 이전에 불리므로, 여기서 세팅한 값은 AIController 빙의/BeginPlay 에서 사용할 수 있다.
	virtual void OnSpawnedBy(AWxSpawner* Spawner);
};
