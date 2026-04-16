// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxElevator.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;
class UWxInteractionWidgetComponent;

/**
 * 엘리베이터.
 * SplineComponent가 정의하는 경로를 따라 플랫폼이 이동한다.
 * 플레이어가 플랫폼 위에서 상호작용하면 이동을 시작하며,
 * 끝에 도달하면 정지하고, 다시 상호작용하면 반대 방향으로 이동한다.
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "Wx"))
class WXWORLD_API AWxElevator : public AActor
{
	GENERATED_BODY()

public:
	AWxElevator();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USplineComponent> SplineComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> PlatformRoot;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionWidgetComponent> InteractionWidget;

	/** 이동 속도 (cm/s) */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0"))
	float MoveSpeed = 200.f;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InteractingActor);

	UFUNCTION()
	void OnRep_bIsMoving();

	void UpdatePlatformPosition();

	UPROPERTY(ReplicatedUsing = OnRep_bIsMoving)
	bool bIsMoving;

	UPROPERTY(Replicated)
	bool bMovingForward = true;

	UPROPERTY(Replicated)
	float CurrentDistance = 0.f;

	float CachedSplineLength;
};
