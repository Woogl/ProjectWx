// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxLaserCorridor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 레이저 벽이 주기적으로 스폰되어 통로를 따라 전진하는 트랩 액터.
 *
 * 레이저 벽 BP(AWxEffectZone 파생)는 ST_LaserCorridor 의 Wx Spawn Actor 에 author 되어 SpawnPoint 컴포넌트의 트랜스폼(위치·회전·스케일)에서 스폰되고, LaserLifetime 후 자동 파괴된다.
 * SpawnPoint 를 통로 입구에 배치하고 벽 크기에 맞춰 스케일하며, 액터 +X(ForwardVector) 방향이 전진 방향이다.
 *
 * 다른 기믹과 동일한 패턴: 권위 State(Active/Disabled)만 C++ 가 소유·확정하며(복제·SaveGame), 스폰·활성 레이저 철거·인터랙션 토글은 전부 GimmickStateTree(ST_LaserCorridor)가 State 태그 이벤트로 진입한 상태에서 적용한다(Wx Spawn Actor / Wx Enable Interaction).
 *
 * 옆 콘솔과 상호작용하면 권위 측이 State 를 Disabled 로 확정해 영구히 비활성화된다(스폰 중단 + 활성 레이저 제거).
 */
UCLASS(Abstract, Blueprintable)
class AWxLaserCorridor : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxLaserCorridor();

	//~ Begin IWxInteractable — 상호작용 시 State 를 Deactivated 로 확정(프롬프트는 베이스 InteractionPrompt).
	virtual void OnInteracted(AActor* Interactor, UActorComponent* Source) override;
	//~ End IWxInteractable

protected:

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Spawn Actor 가 SpawnPoint 로 바인딩하기 위한 노출.
	// 통로 입구에 배치하고 벽 크기에 맞춰 스케일한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SpawnPoint;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> Console;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> ConsoleInteraction;

	// EditAnywhere + AllowPrivateAccess: 디자이너가 인스턴스마다 튜닝하고, StateTree 의 Wx Laser Spawn 이 Context 액터 프로퍼티로 바인딩하기 위한 노출.
	/** 벽 스폰 간격(초). */
	UPROPERTY(EditAnywhere, Category = "Wx|Corridor", meta = (ClampMin = "0.05", AllowPrivateAccess = "true"))
	float SpawnInterval = 2.f;

	/**
	 * 스폰된 레이저 벽의 수명(초).
	 * 통로를 한 번 주파할 시간으로 맞춘다.
	 * Wx Spawn Actor 의 Lifetime 이 바인딩한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|Corridor", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	float LaserLifetime = 4.f;
};
