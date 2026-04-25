// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actor/WxInteractableActor.h"
#include "Spawnable/WxSpawnableInterface.h"
#include "WxTreasureChest.generated.h"

class UStaticMeshComponent;

/**
 * 보물 상자 (테스트용).
 * 플레이어가 InteractionComponent 범위에 진입하면 프롬프트 위젯이 표시되고,
 * 상호작용 입력 시 서버에서 ItemActorClass를 스폰한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxTreasureChest : public AWxInteractableActor, public IWxSpawnableInterface
{
	GENERATED_BODY()

public:
	AWxTreasureChest();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IWxSpawnableInterface
#if WITH_EDITOR
	virtual const UMeshComponent* GetEditorPreviewMeshComponent() const override;
#endif

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 상호작용 시 스폰할 액터 클래스 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TSubclassOf<AActor> ItemActorClass;

	/** 스폰 위치 오프셋 (상자 기준 로컬 좌표) */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FVector SpawnOffset = FVector(0.f, 0.f, 50.f);

	/** 스폰 시 발사 속도 (cm/s) */
	UPROPERTY(EditAnywhere, Category = "Wx|Launch", meta = (ClampMin = "0"))
	float LaunchSpeed = 500.f;

	/** 수직에서 벌어지는 랜덤 각도 범위 (도). 0이면 정확히 위로 */
	UPROPERTY(EditAnywhere, Category = "Wx|Launch", meta = (ClampMin = "0", ClampMax = "90"))
	float LaunchConeHalfAngle = 20.f;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InteractingActor);

	UFUNCTION()
	void OnRep_bIsOpened();

	UPROPERTY(ReplicatedUsing = OnRep_bIsOpened)
	bool bIsOpened = false;
};
