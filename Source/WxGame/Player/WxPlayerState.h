// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "WxPlayerState.generated.h"

class UWxInventoryManagerComponent;

/**
 * 플레이어 세션 단위 상태.
 *
 * 리스폰이나 Pawn 전환에도 유지되어야 하는 데이터(인벤토리/재화 등)를 소유한다.
 * GAS 어트리뷰트는 캐릭터 소유이므로 여기서 관리하지 않는다.
 */
UCLASS()
class WXGAME_API AWxPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AWxPlayerState(const FObjectInitializer& ObjectInitializer);

	UWxInventoryManagerComponent* GetInventoryManager() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Inventory")
	TObjectPtr<UWxInventoryManagerComponent> InventoryManager;
};
