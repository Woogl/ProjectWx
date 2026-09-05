// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "Items/WxRewardTableRow.h"

#include "WxGameFeatureAction_AddInventoryItems.generated.h"

class UWxInventoryComponent;

/**
 * Experience 가 켜질 때 그 월드의 플레이어 인벤토리에 시작 아이템을 지급하는 액션.
 * 활성 시점에 이미 BeginPlay 를 지난 인벤토리엔 즉시, 이후 생기는 인벤토리엔 도착 신호(OnAnyInventoryReady)로 지급한다 — 주입 액션(AddComponents)과의 순서에 의존하지 않는다.
 * 컴포넌트당 도착 신호가 1회라 중복 지급이 없고, 지급은 서버 권한에서만 한다(클라 복제 인스턴스는 건너뜀).
 */
UCLASS(meta = (DisplayName = "Add Inventory Items"))
class UWxGameFeatureAction_AddInventoryItems : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	//~ Begin UGameFeatureAction
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~ End UGameFeatureAction

	/** 빈 항목은 무시된다. */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (TitleProperty = "{Item} x{Quantity}"))
	TArray<FWxItemRewardEntry> Items;

private:
	/** 클래스 차원 신호라 다른 월드(PIE 클라 등)의 인벤토리도 들어온다 — 이 액션의 월드 문맥만 받는다. */
	void HandleInventoryReady(UWxInventoryComponent* Inventory, FGameFeatureStateChangeContext ChangeContext);

	TMap<FGameFeatureStateChangeContext, FDelegateHandle> ReadyHandles;
};
