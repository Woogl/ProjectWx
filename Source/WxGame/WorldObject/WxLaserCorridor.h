// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxLaserCorridor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxLaserCorridorState : uint8
{
	/** 가동 — 초기/기본. 인터랙션 활성, 레이저 주기 스폰·전진. */
	Active,
	/** 정지 — 1회성 발동 완료. 인터랙션 비활성, 스폰 중단 + 활성 레이저 제거. */
	Disabled
};

/**
 * 레이저 벽이 주기적으로 스폰되어 통로를 따라 전진하는 트랩 액터.
 *
 * 통로 영역은 BoxComponent(CorridorBox) 의 BoxExtent 로 정의되며, 액터의 +X(ForwardVector)
 * 방향이 전진 방향이다. 레이저 벽 BP(AWxEffectZone 파생)는 ST_LaserCorridor 의 Wx Laser Spawn
 * 에 author 되어 -X 끝에서 스폰되고 +X 끝에 도달하면 자동 파괴된다.
 *
 * 다른 기믹과 동일한 패턴: 권위 State(Active/Disabled)만 C++ 가 소유·확정하며(복제·SaveGame),
 * 스폰·전진·활성 레이저 철거·인터랙션 토글은 전부 GimmickStateTree(ST_LaserCorridor)가
 * 복제 State 를 Enum Compare 로 추종해 적용한다(Wx Laser Spawn / Wx Laser Advance / Wx Enable Interaction).
 *
 * 옆 콘솔과 상호작용하면 권위 측이 State 를 Disabled 로 확정해 영구히 비활성화된다(스폰 중단 + 활성 레이저 제거).
 */
UCLASS(Abstract, Blueprintable)
class AWxLaserCorridor : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxLaserCorridor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Laser Spawn 이 SpawnVolume 으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CorridorBox;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> Console;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> ConsoleInteraction;

	// EditAnywhere + AllowPrivateAccess: 디자이너가 인스턴스마다 튜닝하고, StateTree 의 Wx Laser Spawn 이 Context 액터 프로퍼티로 바인딩하기 위한 노출.
	/** 벽 스폰 간격(초). */
	UPROPERTY(EditAnywhere, Category = "Wx|Corridor", meta = (ClampMin = "0.05", AllowPrivateAccess = "true"))
	float SpawnInterval = 2.f;

	/** 벽의 전진 속도(cm/s). Wx Laser Spawn 의 수명 산출과 Wx Laser Advance 의 Velocity 가 함께 참조하는 값이다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Corridor", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	float MoveSpeed = 500.f;

private:
	UFUNCTION()
	void HandleConsoleInteracted(AActor* InstigatorActor);

	/** 권위 측에서 State 를 Disabled 로 확정한다. 동일값/비권위면 노옵. 스폰 중단·레이저 철거·인터랙션 토글은 StateTree 가 복제 State 를 추종해 적용한다. */
	void SetLaserCorridorState(EWxLaserCorridorState NewState);

	/**
	 * 트랩 권위/영속 상태. 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종하므로 RepNotify 불필요.
	 * VisibleAnywhere + AllowPrivateAccess 는 StateTree 의 Enum Compare 조건(enter/전이)이 바인딩하기 위한 노출이다.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Wx", Replicated, SaveGame, meta = (AllowPrivateAccess = "true"))
	EWxLaserCorridorState State = EWxLaserCorridorState::Active;
};
