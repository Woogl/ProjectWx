// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "WxPlayerState.generated.h"

/**
 * PlayerState 대상 주입 요청(Experience 액션)의 컴포넌트가 ModularGameplay 컴포넌트 receiver를 통해 자동 부착된다.
 */
UCLASS()
class WXGAME_API AWxPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	//~ Begin AActor
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor
};
