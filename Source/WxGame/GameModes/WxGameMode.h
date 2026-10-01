// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WxGameMode.generated.h"

/**
 * 맵별 구성은 이 클래스의 BP(GM_FrontEnd·GM_Combat)와 맵 WorldSettings 의 GameModeOverride 로 고른다.
 * 플레이어 폰 클래스는 FrontEnd 선택을 우선하고, 선택이 없으면 DefaultPawnClass 를 쓴다(프론트엔드는 SpectatorPawn).
 */
UCLASS()
class AWxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWxGameMode(const FObjectInitializer& ObjectInitializer);

	//~ Begin AGameModeBase
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	//~ End AGameModeBase
};
