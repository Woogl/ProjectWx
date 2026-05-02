// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/WxInteractableActor.h"
#include "WxTreasureChest.generated.h"

class AWxItemPickup;
class UStaticMeshComponent;
class UWxItemDefinition;

/**
 * 보물 상자.
 * 플레이어가 InteractionComponent 범위에 진입하면 프롬프트 위젯이 표시되고,
 * 상호작용 입력 시 서버에서 ItemActorClass(외형) 를 스폰하고 ItemDefinition 을 주입한다.
 */
UCLASS(Abstract)
class AWxTreasureChest : public AWxInteractableActor
{
	GENERATED_BODY()

public:
	AWxTreasureChest();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 드랍되는 아이템의 외형. ItemDefinition 의 지급 데이터는 스폰 후 주입된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TSubclassOf<AWxItemPickup> ItemActorClass;

	/** 스폰된 픽업과 상호작용 시 지급할 아이템 정의. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<UWxItemDefinition> ItemDefinition;

	/** 스폰 시 발사 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Launch", meta = (ClampMin = "0"))
	float LaunchSpeed = 300.f;

	/** 수직에서 벌어지는 랜덤 각도 범위 (도). 0이면 정확히 위로 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Launch", meta = (ClampMin = "0", ClampMax = "90"))
	float LaunchConeHalfAngle = 20.f;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InteractingActor);

	UFUNCTION()
	void OnRep_bIsOpened();

	UPROPERTY(ReplicatedUsing = OnRep_bIsOpened)
	bool bIsOpened = false;
};
