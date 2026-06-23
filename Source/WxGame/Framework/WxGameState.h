// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "WxGameState.generated.h"

/**
 * 프로젝트 공용 GameState.
 *
 * ModularGameplay 컴포넌트 receiver 다. 생성자에서 컴포넌트를 직접 만들지 않고, GameMode 가 요청 등록한 프레임워크
 * 컴포넌트(TimeDilation·PlayerSpawning 등)를 자동 주입받는다. 따라서 GameState 는 어떤 컴포넌트가 붙는지 알지 않는다.
 */
UCLASS()
class WXGAME_API AWxGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	//~ Begin AActor
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor
};
