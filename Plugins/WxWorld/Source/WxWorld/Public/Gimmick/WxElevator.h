// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxElevator.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 엘리베이터.
 * SplineComponent가 정의하는 경로를 따라 플랫폼이 이동한다.
 * 인터랙션 영역은 셋:
 *  - PlatformInteraction: 플랫폼 위에서 상호작용하면 진행/방향 토글로 이동 시작
 *  - CallConsoleAInteraction / CallConsoleBInteraction: 해당 콘솔이 위치한 층(스플라인 끝점)으로 엘리베이터를 호출
 * 콘솔 끝점 매핑은 BeginPlay에서 콘솔 월드 위치와 스플라인 양 끝점 거리를 비교해 자동 결정한다.
 */
UCLASS(Abstract)
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
	TObjectPtr<UWxInteractionComponent> PlatformInteraction;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> CallConsoleA;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> CallConsoleAInteraction;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> CallConsoleB;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> CallConsoleBInteraction;

	/** 이동 속도 (cm/s) */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0"))
	float MoveSpeed = 200.f;

private:
	UFUNCTION()
	void HandlePlatformInteracted(AActor* InteractingActor);

	UFUNCTION()
	void HandleCallConsoleAInteracted(AActor* InteractingActor);

	UFUNCTION()
	void HandleCallConsoleBInteracted(AActor* InteractingActor);

	UFUNCTION()
	void OnRep_bIsMoving();

	void StartMovementToDistance(float TargetDistance);

	void UpdatePlatformPosition();

	void SetAllInteractionsEnabled(bool bEnabled);

	UPROPERTY(ReplicatedUsing = OnRep_bIsMoving)
	bool bIsMoving = false;

	UPROPERTY(Replicated)
	bool bMovingForward = true;

	UPROPERTY(Replicated)
	float CurrentDistance = 0.f;

	float CachedSplineLength;

	float CallConsoleATargetDistance;

	float CallConsoleBTargetDistance;
};
