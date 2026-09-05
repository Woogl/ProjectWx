// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxRespawnLibrary.generated.h"

class UCommonActivatableWidget;

UCLASS()
class WXGAME_API UWxRespawnLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** DeathScreen에는 호출 위젯 Self를 전달한다. 즉시 부활 성공 시 화면을 닫고 true를 반환한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Respawn")
	static bool RequestRespawn(UCommonActivatableWidget* DeathScreen);
};
