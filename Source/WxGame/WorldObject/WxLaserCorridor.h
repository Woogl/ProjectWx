// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxLaserCorridor.generated.h"

class AWxEffectZone;
class UBoxComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxLaserCorridorState : uint8
{
	/** 가동 — 초기/기본. 인터랙션 활성, 스폰 타이머 가동. */
	Active,
	/** 정지 — 1회성 발동 완료. 인터랙션 비활성, 타이머 중단 + 활성 레이저 제거. */
	Disabled
};

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
 * 상태는 자체 EWxLaserCorridorState(State) 가 권위 원천이며, 복제·SaveGame 으로 보존된다.
 */
UCLASS(Abstract, Blueprintable)
class AWxLaserCorridor : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxLaserCorridor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin IWxSavable — base 가 더 이상 복원 후크를 제공하지 않으므로 자체적으로 상태를 동기화한다.
	virtual void OnWxSaveRestored() override;
	//~ End IWxSavable

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

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

	UFUNCTION()
	void OnRep_State();

	/** 권위 측에서 State 를 전환하고 RefreshLaserState 로 적용. 동일값/비권위면 노옵. */
	void SetLaserCorridorState(EWxLaserCorridorState NewState);

	/** 현재 State 를 인터랙션/스폰 타이머/활성 레이저에 적용한다. BeginPlay·OnRep·권위 setter·WxSave 복원에서 호출. */
	void RefreshLaserState();

	void HandleSpawnTimer();

	FTimerHandle SpawnTimerHandle;

	TArray<TWeakObjectPtr<AWxEffectZone>> ActiveLasers;

	/** 트랩 권위/영속 상태. 클라는 OnRep_State 로 동기화된다. */
	UPROPERTY(ReplicatedUsing = OnRep_State, SaveGame)
	EWxLaserCorridorState State = EWxLaserCorridorState::Active;
};
