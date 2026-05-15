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
	/** 정지 — 초기 한 번만 진입 (dead-state). 문 닫힘. 게임 진행 중 재진입하지 않는다. */
	DoorsClosed,
	/** 전이 — 목적지 끝점에서 문 여는 중. Tick 활성, 인터랙션 비활성. */
	DoorsOpening,
	/** 정지 — 도착 후 안정. 게임 진행 중 모든 정지는 여기로 수렴. */
	DoorsOpen,
	/** 전이 — 출발 끝점에서 문 닫는 중. Tick 활성, 인터랙션 비활성. */
	DoorsClosing,
	/** 전이 — 출발 끝점 → 목적지 끝점 이동 중. Tick 활성, 인터랙션 비활성. */
	Moving
};

/** 엘리베이터가 정지·이동 시 향하는 스플라인 끝점. Start=거리 0, End=SplineLength. */
UENUM()
enum class EWxElevatorEndpoint : uint8
{
	Start,
	End
};

/**
 * 엘리베이터.
 * SplineComponent 가 정의하는 경로를 따라 플랫폼이 이동한다.
 *
 * 상태 머신 — 5 개 상태가 2 개의 정지(stable) + 3 개의 전이(transition) 로 나뉜다:
 *   정지 (인터랙션 활성, Tick 비활성):
 *     DoorsClosed  초기 한 번만 진입. 한 번 DoorsOpen 으로 가면 게임 진행 중 재진입 안 함 (dead-state).
 *     DoorsOpen    도착 후 안정 상태. 게임 진행 중 정지는 항상 여기로 수렴.
 *   전이 (인터랙션 비활성, Tick 활성):
 *     DoorsOpening 목적지 끝점에서 문 여는 중.
 *     DoorsClosing 출발 끝점에서 문 닫는 중.
 *     Moving       출발 끝점 → 목적지 끝점 이동 중.
 *
 * 호출별 전이 흐름:
 *   DoorsClosed + 같은 끝점 호출 → DoorsOpening → DoorsOpen
 *   DoorsClosed + 다른 끝점 호출 → Moving (DoorsClosing 스킵, 이미 닫혀 있음) → DoorsOpening → DoorsOpen
 *   DoorsOpen + 같은 끝점 호출 → 노옵
 *   DoorsOpen + 다른 끝점 호출 → DoorsClosing → Moving → DoorsOpening → DoorsOpen
 *
 * 위치 정보:
 *   State + TargetEndpoint(슬롯 보존) 두 값으로 모든 위치가 결정된다 — CurrentDistance/TargetDistance 는 ApplyState 에서 매번 스냅.
 *   정지·DoorsOpening 상태의 현재 위치 = TargetEndpoint. DoorsClosing·Moving 의 현재 위치 = TargetEndpoint 의 반대(출발 끝점).
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

	UFUNCTION()
	void OnRep_TargetEndpoint();

	/** 시작점(거리 0)으로 이동 시퀀스 개시. 서버에서만 동작. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	void MovePlatformToStart();

	/** 끝점(SplineLength)으로 이동 시퀀스 개시. 서버에서만 동작. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	void MovePlatformToEnd();

	void BeginMoveSequence(EWxElevatorEndpoint NewEndpoint);

	void UpdatePlatformPosition();

	/** DoorAnimProgress 에 따라 양쪽 문 위치를 lerp. 각 문은 자신의 너비만큼 바깥쪽으로 슬라이드. */
	void UpdateDoorPositions();

	void SetAllInteractionsEnabled(bool bEnabled);

	/** 문 메시의 로컬 Y 축 너비(스케일 반영). 표준 UE 도어 메시는 Y 가 너비 축. */
	float ComputeDoorWidth(const UStaticMeshComponent* DoorMesh) const;

	UPROPERTY(ReplicatedUsing = OnRep_State, SaveGame)
	EWxElevatorState State = EWxElevatorState::DoorsClosed;

	/**
	 * 현재 정지 또는 이동 중인 목표 끝점. State 와 함께 슬롯에 보존되어 복원 시 위치가 일관되게 결정된다.
	 * RepNotify 인 이유: State 와 같은 frame 에 변경될 때 property 적용 순서가 보장되지 않으므로,
	 * 어느 게 먼저 와도 마지막 도착 시 ApplyState 가 두 값이 모두 set 된 상태로 한 번 더 호출되도록 보정.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_TargetEndpoint, SaveGame)
	EWxElevatorEndpoint TargetEndpoint = EWxElevatorEndpoint::Start;

	/** TargetEndpoint 가 결정한 목표 스플라인 거리. ApplyState 에서 endpoint 기반으로 매번 스냅된다. */
	UPROPERTY(Replicated)
	float TargetDistance = 0.f;

	UPROPERTY(Replicated)
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
