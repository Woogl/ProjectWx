// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/DataTable.h"
#include "WxRewardComponent.generated.h"

class UArrowComponent;

/**
 * 보상 지급 컴포넌트.
 * RewardRow 의 유효한 (아이템, 수량) 항목마다 Pickup Fragment 가 지정한 픽업 액터를 스폰해 흩뿌린다.
 * 컴포넌트의 월드 위치가 스폰 위치, 업 벡터가 발사 원뿔의 중심축이 된다 — 오너 BP에서 배치/회전으로 드랍 위치와 방향을 조절한다.
 * Pickup Fragment 가 없는 아이템(예: 재화)은 외형이 없으므로 DirectGrantTarget 의 인벤토리에 직접 지급하고, 대상이 없으면 스킵한다.
 *
 * 상호작용 오너(예: WxWorld 의 보물 상자) 지원:
 * BeginPlay 에서 오너의 IWxInteractionSource 구현 컴포넌트를 자동으로 찾아 바인딩하고, 상호작용 시 Instigator 를
 * DirectGrantTarget 으로 보상을 지급한다. 1회성 게이팅은 오너가 상호작용 활성/비활성으로 제어한다.
 * 상호작용과 무관한 오너(예: 적 사망 드랍)는 DropRewards 를 직접 호출한다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXINVENTORY_API UWxRewardComponent : public USceneComponent
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	virtual void OnRegister() override;
#endif

	/**
	 * 보상을 지급한다. 서버 권한에서만 동작한다.
	 * @param DirectGrantTarget Pickup Fragment 없는 아이템을 인벤토리에 직접 지급할 대상. nullptr 이면 해당 아이템은 스킵된다.
	 */
	void DropRewards(AActor* DirectGrantTarget = nullptr);

protected:
	virtual void BeginPlay() override;

	/**
	 * 지급할 보상. FWxRewardTableRow 로우를 가리킨다.
	 * 유효한 (아이템, 수량) 항목마다 픽업이 하나씩 스폰되어 발사된다.
	 * 비워 두면 아무것도 지급하지 않는다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (RowType = "/Script/WxInventory.WxRewardTableRow"))
	FDataTableRowHandle RewardRow;

	/** 스폰 시 발사 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Launch", meta = (ClampMin = "0"))
	float LaunchSpeed = 300.f;

	/** 컴포넌트 업 벡터에서 벌어지는 랜덤 각도 범위 (도). 0이면 정확히 업 벡터 방향으로 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Launch", meta = (ClampMin = "0", ClampMax = "90"))
	float LaunchConeHalfAngle = 20.f;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);

#if WITH_EDITORONLY_DATA
	/** 에디터에서 발사 축(업 벡터)을 표시하는 화살표. 게임에는 표시되지 않는다. */
	UPROPERTY(Transient)
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif
};
