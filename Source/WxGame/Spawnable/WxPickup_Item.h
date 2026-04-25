// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Spawnable/WxPickupBase.h"
#include "WxPickup_Item.generated.h"

class UWxItemDefinition;

/**
 * 아이템(또는 재화) 지급용 픽업.
 *
 * 상호작용 시 Interactor 의 PlayerState(또는 본체) 에서
 * UWxInventoryManagerComponent 를 찾아 ItemDef 를 Count 만큼 지급한 뒤 파괴된다.
 */
UCLASS()
class AWxPickup_Item : public AWxPickupBase
{
	GENERATED_BODY()

protected:
	virtual void OnPickedUp(AActor* InteractingActor) override;

	/** 지급할 아이템 정의. Currency Fragment 가 붙은 재화도 동일하게 사용. */
	UPROPERTY(EditAnywhere, Category = "Wx|Pickup")
	TObjectPtr<UWxItemDefinition> ItemDef;

	/** 지급 수량. */
	UPROPERTY(EditAnywhere, Category = "Wx|Pickup", meta = (ClampMin = "1"))
	int32 Count = 1;
};
