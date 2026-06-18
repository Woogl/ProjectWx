// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxElevator.generated.h"

class USplineComponent;
class UStateTreeComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxElevatorState : uint8
{
	/** 정지 — 초기 한 번만 진입 (dead-state). 문 닫힘. 게임 진행 중 재진입하지 않는다. */
	DoorsClosed,
	/** 전이 — 목적지 끝점에서 문 여는 중. 인터랙션 비활성. 문 애니 완료 시 서버가 DoorsOpen 으로 승급. */
	DoorsOpening,
	/** 정지 — 도착 후 안정. 게임 진행 중 모든 정지는 여기로 수렴. */
	DoorsOpen,
	/** 전이 — 출발 끝점에서 문 닫는 중. 인터랙션 비활성. 문 애니 완료 시 서버가 Moving 으로 승급. */
	DoorsClosing,
	/** 전이 — 출발 끝점 → 목적지 끝점 이동 중. 인터랙션 비활성. 도착 시 서버가 DoorsOpening 으로 승급. */
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
 *   정지 (인터랙션 활성):
 *     DoorsClosed  초기 한 번만 진입. 한 번 DoorsOpen 으로 가면 게임 진행 중 재진입 안 함 (dead-state).
 *     DoorsOpen    도착 후 안정 상태. 게임 진행 중 정지는 항상 여기로 수렴.
 *   전이 (인터랙션 비활성):
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
 * 상태는 자체 EWxElevatorState(State) 가 권위 원천이며, TargetEndpoint 와 함께 복제·SaveGame 으로 보존된다.
 * StateTree(ElevatorStateTree)는 State 를 읽어 비주얼을 렌더하고 전이를 구동하는 상태머신이며, 상태·전이는 ST_Elevator 에셋에서 author 한다.
 * 전이는 "권위 State 변경 → 전이 태스크가 State 변화를 감지해 Succeeded 반환 → On Succeeded → Root 재선택" 으로 구동된다(이벤트 태그 없음).
 *   - 콘솔/플랫폼 상호작용(서버)이 BeginMoveSequence 로 DoorsClosing/Moving/DoorsOpening 을 개시.
 *   - 전이 태스크(ElevatorMove/ElevatorDoorPose)가 보간 완료 시 서버에서 SetElevatorState 로 다음 상태로 승급 → 같은 태스크가 State 변화를 감지해 완료.
 *   - 클라는 로컬 완료해도 승급은 노옵이라, 복제로 State 가 바뀐 뒤에야 완료(서버가 전이의 클럭).
 *
 * 위치 정보:
 *   State + TargetEndpoint(슬롯 보존) 두 값으로 모든 위치가 결정된다 — CurrentDistance/TargetDistance/문 알파는 SnapVisualsToState 가 스냅.
 *   StateTree 는 도어와 달리 위치 스냅을 들지 않는다(플랫폼 위치 상태가 풍부해 복원/복제 정합을 위해 ApplyState 의 SnapVisualsToState 가 스냅을 유지).
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

	//~ Begin StateTree 노드가 호출하는 프리미티브
	/** 세 인터랙션(플랫폼/콘솔 A/콘솔 B) 일괄 활성/비활성. */
	void SetAllInteractionsEnabled(bool bEnabled);

	/** 플랫폼을 스플라인 거리(Distance)로 이동시키고 CurrentDistance 갱신. */
	void SetPlatformDistance(float Distance);

	/** 현재 플랫폼 스플라인 거리. ElevatorMove 가 현재→목표 보간의 시작점으로 읽는다. */
	float GetPlatformDistance() const { return CurrentDistance; }

	/** 이번 이동의 목표 스플라인 거리(최종 끝점). */
	float GetTargetDistance() const { return TargetDistance; }

	/** 한 끝점 → 다른 끝점 이동에 걸리는 시간(초). */
	float GetMoveDuration() const { return MoveDuration; }

	/** 캐시된 스플라인 전체 길이. */
	float GetSplineLength() const { return CachedSplineLength; }

	/** 문 개방 알파(0=닫힘, 1=열림)로 양쪽 문 위치를 갱신. */
	void SetDoorOpenAlpha(float Alpha);

	/** 현재 문 개방 알파. ElevatorDoorPose 가 현재→목표 보간의 시작점으로 읽는다. */
	float GetDoorOpenAlpha() const { return CurrentOpenAlpha; }

	/** 문 슬라이드 애니메이션 길이(초). */
	float GetDoorAnimDuration() const { return DoorAnimDuration; }

	/** 현재 엘리베이터 상태. StateTree 의 ElevatorStateIs 조건이 상태 선택에 사용. */
	EWxElevatorState GetElevatorState() const { return State; }

	/** 권위 측에서 State 를 전환하고 ApplyState 로 StateTree 를 재선택. 동일값/비권위면 노옵. 전이 태스크의 완료 승급에 사용. */
	void SetElevatorState(EWxElevatorState NewState);
	//~ End StateTree 노드가 호출하는 프리미티브

protected:
	virtual void BeginPlay() override;
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

	/** 엘리베이터 상태 머신을 구동하는 StateTree. 실행할 ST_Elevator 에셋은 BP_Elevator 에서 할당한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStateTreeComponent> ElevatorStateTree;

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

	/** 현재 State + TargetEndpoint 로 플랫폼 거리·문 알파를 스냅한다. BeginPlay pre-snap·ApplyState 가 호출. */
	void SnapVisualsToState();

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

	/** TargetEndpoint 가 결정한 목표 스플라인 거리. SnapVisualsToState 에서 endpoint 기반으로 매번 스냅된다. */
	UPROPERTY(Replicated)
	float TargetDistance = 0.f;

	UPROPERTY(Replicated)
	float CurrentDistance = 0.f;

	float CachedSplineLength;

	/** 현재 적용된 문 개방 알파(0=닫힘, 1=열림). 각 머신에서 ElevatorDoorPose 가 로컬 누적 (복제 없음). */
	float CurrentOpenAlpha = 0.f;

	FVector DoorLeftClosedLocation;
	FVector DoorRightClosedLocation;

	/** 각 문 메시의 Y 너비만큼 자기 바깥쪽 방향(좌: -Y, 우: +Y)으로의 슬라이드 오프셋. */
	FVector DoorLeftOpenOffset;
	FVector DoorRightOpenOffset;
};
