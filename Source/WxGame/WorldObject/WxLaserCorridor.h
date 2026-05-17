// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxLaserCorridor.generated.h"

class AWxEffectZone;
class UBoxComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 레이저 벽이 주기적으로 스폰되어 통로를 따라 전진하는 트랩 액터.
 *
 * 통로 영역은 BoxComponent(CorridorVolume) 의 BoxExtent 로 정의되며, 액터의 +X(ForwardVector)
 * 방향이 전진 방향이다. 레이저 벽은 AWxEffectZone 의 Blueprint 클래스(LaserZoneClass) 를
 * 그대로 활용하며 -X 끝에서 스폰되어 +X 끝에 도달하면 자동 파괴된다.
 *
 * 벽의 외형(Niagara), 박스 콜리전, 적용할 GE(EffectClass/SetByCallers/EffectAssetTags) 는
 * LaserZoneClass(Blueprint) 의 클래스 디폴트에서 설정한다. 통로 액터는 스폰 → 자체 Tick
 * 이동 → 수명 종료 시 파괴를 담당한다.
 *
 * 옆에 배치된 콘솔과 상호작용하면 영구히 비활성화된다(스폰 타이머 중단 + 활성 레이저 즉시 제거).
 * 발동 후 상태는 bTriggered(AWxGimmick) 로 보존된다.
 */
UCLASS(Abstract, Blueprintable)
class AWxLaserCorridor : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxLaserCorridor();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void ApplyState() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UBoxComponent> CorridorBox;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> Console;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> ConsoleInteraction;

	/** 스폰될 레이저 벽(EffectZone) 클래스. Blueprint 에서 외형/콜리전/적용 GE 를 구성한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Corridor")
	TSubclassOf<AWxEffectZone> LaserZoneClass;

	/** 벽 스폰 간격(초). */
	UPROPERTY(EditAnywhere, Category = "Wx|Corridor", meta = (ClampMin = "0.05"))
	float SpawnInterval = 2.f;

	/** 벽의 전진 속도(cm/s). */
	UPROPERTY(EditAnywhere, Category = "Wx|Corridor", meta = (ClampMin = "0"))
	float MoveSpeed = 500.f;

private:
	UFUNCTION()
	void HandleConsoleInteracted(AActor* InstigatorActor);

	void HandleSpawnTimer();

	FTimerHandle SpawnTimerHandle;

	TArray<TWeakObjectPtr<AWxEffectZone>> ActiveLasers;
};
