// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "WxGameState.generated.h"

class UWxTimeDilationComponent;

/**
 * 프로젝트 공용 GameState.
 *
 * 서버 권위 Global TimeDilation 관리를 위한 UWxTimeDilationComponent를 부착한다.
 */
UCLASS()
class WXGAME_API AWxGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AWxGameState();

private:
	UPROPERTY(VisibleAnywhere, Category = "Wx|Time")
	TObjectPtr<UWxTimeDilationComponent> TimeDilationComponent;
};
