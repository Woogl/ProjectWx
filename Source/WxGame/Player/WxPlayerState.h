// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "WxPlayerState.generated.h"

/**
 * ModularGameplay 컴포넌트 receiver 다 — PlayerState 대상 주입 요청(Experience 액션)의 컴포넌트가 자동 부착된다.
 * 인벤토리는 PlayerController 로 이동했다(소유 클라이언트 단위 복제 + UE 표준 정렬).
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
