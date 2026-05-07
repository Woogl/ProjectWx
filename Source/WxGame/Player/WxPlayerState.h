// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "WxPlayerState.generated.h"

/**
 * 플레이어 세션 단위 상태.
 *
 * 인벤토리는 PlayerController 로 이동했다(소유 클라이언트 단위 복제 + UE 표준 정렬).
 * 본 클래스는 향후 모든 클라이언트에 공유되어야 하는 세션 상태(스코어/팀 등)의 거주처이다.
 */
UCLASS()
class WXGAME_API AWxPlayerState : public APlayerState
{
	GENERATED_BODY()
};
