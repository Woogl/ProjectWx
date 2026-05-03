// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameState.h"

#include "Time/WxTimeDilationComponent.h"

AWxGameState::AWxGameState()
{
	TimeDilationComponent = CreateDefaultSubobject<UWxTimeDilationComponent>(TEXT("TimeDilationComponent"));
}
