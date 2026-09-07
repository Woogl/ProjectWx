// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Items/WxRewardTableRow.h"
#include "WxGameMode.generated.h"

/**
 * 맵별 구성은 이 클래스의 BP(GM_FrontEnd·GM_Combat)와 맵 WorldSettings 의 GameModeOverride 로 고른다.
 * 플레이어 폰 클래스는 FrontEnd 선택을 우선하고, 선택이 없으면 DefaultPawnClass 를 쓴다(프론트엔드는 SpectatorPawn).
 * 시작 아이템은 시작 플레이어 처리에서 스폰 뒤 컨트롤러의 인벤토리에 직접 지급한다.
 */
UCLASS()
class AWxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWxGameMode(const FObjectInitializer& ObjectInitializer);

	//~ Begin AGameModeBase
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	//~ End AGameModeBase

protected:
	/** 접속한 플레이어의 인벤토리에 한 번 지급한다. 빈 항목은 무시된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (TitleProperty = "{Item} x{Quantity}"))
	TArray<FWxItemRewardEntry> StartingItems;
};
