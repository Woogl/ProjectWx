// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Gimmick/WxGimmick.h"
#include "WxTreasureChest.generated.h"

class AWxItemPickup;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 보물 상자.
 * 플레이어가 InteractionComponent 범위에 진입하면 프롬프트 위젯이 표시되고,
 * 상호작용 입력 시 서버에서 RewardRow 의 유효한 (아이템, 수량) 항목마다 ItemActorClass(외형) 를
 * 스폰하고 해당 아이템 정의/수량을 주입한 뒤 상자 위로 흩뿌린다.
 */
UCLASS(Abstract)
class AWxTreasureChest : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxTreasureChest();

protected:
	virtual void BeginPlay() override;
	virtual void ApplyState() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	/** 드랍되는 아이템의 외형. 보상 항목의 지급 데이터는 스폰 후 주입된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<AWxItemPickup> ItemActorClass;

	/**
	 * 지급할 보상. FWxRewardTableRow 로우를 가리킨다.
	 * 유효한 (아이템, 수량) 항목마다 픽업이 하나씩 스폰되어 상자 위로 발사된다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxInventory.WxRewardTableRow"))
	FDataTableRowHandle RewardRow;

	/** 스폰 시 발사 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Launch", meta = (ClampMin = "0"))
	float LaunchSpeed = 300.f;

	/** 수직에서 벌어지는 랜덤 각도 범위 (도). 0이면 정확히 위로 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Launch", meta = (ClampMin = "0", ClampMax = "90"))
	float LaunchConeHalfAngle = 20.f;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);
};
