// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "WxPlayerState.generated.h"

/**
 * 플레이어 단위 상태는 아직 없다 — 스탯은 캐릭터 ASC 가 들고 리스폰마다 새로 초기화한다. GameMode 가 PlayerStateClass 로 지정한다.
 */
UCLASS()
class WXGAME_API AWxPlayerState : public APlayerState
{
	GENERATED_BODY()
};
