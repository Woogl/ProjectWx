// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
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
class WXWORLD_API AWxElevator : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxElevator();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void ApplyState() override;

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

	void BeginMoveSequence(float NewTargetDistance);

	void UpdatePlatformPosition();

	/** DoorAnimProgress 에 따라 양쪽 문 위치를 lerp. 각 문은 자신의 너비만큼 바깥쪽으로 슬라이드. */
	void UpdateDoorPositions();

	void SetAllInteractionsEnabled(bool bEnabled);

	/** 문 메시의 로컬 Y 축 너비(스케일 반영). 표준 UE 도어 메시는 Y 가 너비 축. */
	float ComputeDoorWidth(const UStaticMeshComponent* DoorMesh) const;

	UPROPERTY(ReplicatedUsing = OnRep_State, SaveGame)
	EWxElevatorState State = EWxElevatorState::DoorsClosed;

	/** 현재 이동 시퀀스의 목표 스플라인 거리. DoorsOpen/DoorsClosed 정지 시엔 현재 위치와 동일. */
	UPROPERTY(Replicated, SaveGame)
	float TargetDistance = 0.f;

	UPROPERTY(Replicated, SaveGame)
	float CurrentDistance = 0.f;

	float CachedSplineLength;

	/** 문 애니 진행도. 0=닫힘, 1=열림. 각 머신에서 Tick 으로 로컬 누적 (복제 없음). */
	float DoorAnimProgress = 0.f;

	FVector DoorLeftClosedLocation;
	FVector DoorRightClosedLocation;

	/** 각 문 메시의 Y 너비만큼 자기 바깥쪽 방향(좌: -Y, 우: +Y)으로의 슬라이드 오프셋. */
	FVector DoorLeftOpenOffset;
	FVector DoorRightOpenOffset;
};
