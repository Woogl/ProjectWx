// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxElevator.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxElevatorState : uint8
{
	DoorsClosed,
	DoorsOpening,
	DoorsOpen,
	DoorsClosing,
	Moving
};

/**
 * 엘리베이터.
 * SplineComponent가 정의하는 경로를 따라 플랫폼이 이동한다.
 *
 * 상태 머신:
 *   DoorsClosed (초기) → 호출 시 → DoorsOpening → DoorsOpen
 *   DoorsOpen → 이동 명령 시 → DoorsClosing → Moving → 도착 → DoorsOpening → DoorsOpen
 *
 * DoorsClosed 와 DoorsOpen 둘 다 정지/입력 수락 상태이며, 문 시각 상태로 구분된다.
 *
 * 인터랙션 영역은 셋:
 *  - PlatformInteraction: 플랫폼 위에서 상호작용하면 반대 끝점으로 이동 시작 (DoorsOpen 일 때만)
 *  - CallConsoleAInteraction: 플랫폼을 스플라인 시작점(거리 0)으로 호출
 *  - CallConsoleBInteraction: 플랫폼을 스플라인 끝점(SplineLength)으로 호출
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
	TObjectPtr<UStaticMeshComponent> DoorLeft;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> DoorRight;

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

	/** 한 끝점에서 다른 끝점까지 이동하는 데 걸리는 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0"))
	float MoveDuration = 3.f;

	/** 문 열림/닫힘 애니메이션 길이(초). */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0"))
	float DoorAnimDuration = 1.f;

	/** 문이 완전히 열렸을 때 DoorRight 의 닫힘 위치 기준 오프셋. DoorLeft 는 반대 방향으로 동일한 거리만큼 슬라이드. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FVector DoorOpenOffset = FVector(100.f, 0.f, 0.f);

private:
	UFUNCTION()
	void HandlePlatformInteracted(AActor* InteractingActor);

	UFUNCTION()
	void HandleCallConsoleAInteracted(AActor* InteractingActor);

	UFUNCTION()
	void HandleCallConsoleBInteracted(AActor* InteractingActor);

	UFUNCTION()
	void OnRep_State();

	/** 시작점(거리 0)으로 이동 시퀀스 개시. 서버에서만 동작. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	void MovePlatformToStart();

	/** 끝점(SplineLength)으로 이동 시퀀스 개시. 서버에서만 동작. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	void MovePlatformToEnd();

	void BeginMoveSequence(float TargetDistance);

	/** 현재 State 값을 런타임(틱/인터랙션/위치 스냅) 에 적용. 서버 상태 변경 시점과 클라이언트 OnRep 양쪽에서 호출. */
	void ApplyState();

	void UpdatePlatformPosition();

	/** DoorAnimProgress 에 따라 양쪽 문 위치를 lerp. DoorRight 는 +DoorOpenOffset, DoorLeft 는 -DoorOpenOffset 방향. */
	void UpdateDoorPositions();

	void SetAllInteractionsEnabled(bool bEnabled);

	UPROPERTY(ReplicatedUsing = OnRep_State)
	EWxElevatorState State = EWxElevatorState::DoorsClosed;

	UPROPERTY(Replicated)
	bool bMovingForward = true;

	UPROPERTY(Replicated)
	float CurrentDistance = 0.f;

	float CachedSplineLength;

	/** 문 애니 진행도. 0=닫힘, 1=열림. 각 머신에서 Tick 으로 로컬 누적 (복제 없음). */
	float DoorAnimProgress = 0.f;

	FVector DoorLeftClosedLocation;
	FVector DoorRightClosedLocation;
};
