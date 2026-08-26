// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxSpawnerLibrary.generated.h"

/**
 * 서버 권위 호출만 의미가 있다. 클라에서 호출되면 Spawner 가 내부에서 무시한다.
 */
UCLASS()
class WXWORLD_API UWxSpawnerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Manual 모드는 제외. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Spawner", meta = (WorldContext = "WorldContextObject"))
	static void TryRespawnAll(const UObject* WorldContextObject);
};
