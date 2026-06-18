// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxElevator.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "WxElevatorStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

/**
 * AWxElevator 의 StateTree 구동용 노드 모음.
 * 상태·전이는 ST_Elevator 에셋에서 author 하며, 여기의 노드는 소유 AWxElevator 의 얇은 프리미티브
 * (플랫폼 이동/문 포즈/인터랙션 토글/State 조회·승급) 만 호출한다. 컨텍스트 액터는 StateTreeComponentSchema 가
 * 제공하는 소유 액터(= AWxElevator) 이며, 각 노드는 Context.GetOwner() 를 캐스트해 얻는다.
 *
 * 전이는 도어와 동일하게 "권위 State 변경 → 전이 태스크가 State 변화를 감지해 Succeeded → On Succeeded → Root 재선택" 으로 구동된다(이벤트 태그 없음, 서버가 클라 전이를 게이팅).
 * 노드는 넷이다(직교 단일 책임):
 *  - ElevatorMove 는 플랫폼을 현재 거리에서 목표 거리로 MoveDuration 기준 일정 속도 보간하고, 도달 시 서버가 PromoteToState 로 승급한다.
 *  - ElevatorDoorPose 는 (bOpen) 으로 문 알파를 현재→목표 보간하고, 도달 시 서버가 PromoteToState 로 승급한다(정지 상태는 자기상태 승급=노옵으로 hold).
 *  - ElevatorInteraction 은 (bEnableInteraction) 으로 세 인터랙션을 일괄 토글.
 *  - ElevatorStateIs 는 (State) 로 각 비주얼 상태의 enter condition 에 재사용.
 */

// ── ElevatorMove: 플랫폼 이동 ──────────────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_ElevatorMoveInstanceData
{
	GENERATED_BODY()

	/** 플랫폼이 목표 거리에 도달했을 때 서버가 승급할 다음 상태(보통 DoorsOpening). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	EWxElevatorState PromoteToState = EWxElevatorState::DoorsOpening;

	/** EnterState 시점의 권위 State(런타임 전용). 권위 State 가 이 값에서 벗어나면 Succeeded 를 반환해 재선택을 유발. */
	UPROPERTY()
	EWxElevatorState EnteredState = EWxElevatorState::Moving;
};

/**
 * Tick 으로 플랫폼 스플라인 거리를 현재값에서 목표(GetTargetDistance)로 MoveDuration 에 맞춘 일정 속도로 보간하고,
 * 도달 시 서버가 SetElevatorState 로 PromoteToState 로 승급한다. 전이는 "권위 State 가 진입 시점(EnteredState)에서
 * 벗어나면 Succeeded → On Succeeded → Root 재선택"으로 구동(클라는 복제로 State 가 바뀐 뒤 완료, 서버가 게이팅).
 * 시작 시 이미 목표 거리면(SnapVisualsToState 가 State 에 맞게 pre-snap) 움직임 없이 즉시 스냅·승급된다.
 */
USTRUCT(meta = (DisplayName = "Wx Elevator Move"))
struct FWxStateTreeTask_ElevatorMove : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_ElevatorMoveInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── ElevatorDoorPose: 현재→목표 보간 문 포즈 ───────────────────────────────

USTRUCT()
struct FWxStateTreeTask_ElevatorDoorPoseInstanceData
{
	GENERATED_BODY()

	/** 목표 포즈가 열림인지(true=열림 1, false=닫힘 0). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bOpen = false;

	/** 문 알파가 목표에 도달했을 때 서버가 승급할 상태. 정지 상태는 자기상태로 두어 승급 없이 hold. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	EWxElevatorState PromoteToState = EWxElevatorState::DoorsOpen;

	/** EnterState 시점의 권위 State(런타임 전용). 권위 State 가 이 값에서 벗어나면 Succeeded 를 반환해 재선택을 유발. */
	UPROPERTY()
	EWxElevatorState EnteredState = EWxElevatorState::DoorsClosed;
};

/**
 * Tick 으로 문 개방 알파를 현재값에서 목표(bOpen?1:0)로 DoorAnimDuration 에 맞춘 일정 속도로 보간하고,
 * 도달 시 서버가 SetElevatorState 로 PromoteToState 로 승급한다(정지 상태는 자기상태 승급이라 노옵, 그대로 hold).
 * 전이는 "권위 State 가 진입 시점(EnteredState)에서 벗어나면 Succeeded → On Succeeded → Root 재선택"으로 구동
 * (정지 상태는 상호작용이 State 를 바꿀 때 완료, 클라는 복제로 State 가 바뀐 뒤 완료, 서버가 게이팅).
 * 시작 시 이미 목표 포즈면(SnapVisualsToState 가 State 에 맞게 pre-snap) 움직임 없이 즉시 스냅된다.
 */
USTRUCT(meta = (DisplayName = "Wx Elevator Door Pose"))
struct FWxStateTreeTask_ElevatorDoorPose : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_ElevatorDoorPoseInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── ElevatorInteraction: 인터랙션 일괄 토글 ────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_ElevatorInteractionInstanceData
{
	GENERATED_BODY()

	/** 진입 시 세 인터랙션(플랫폼/콘솔 A/콘솔 B) 활성 여부. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnableInteraction = false;
};

/**
 * 진입 시 세 인터랙션을 지정값으로 일괄 토글하고 머문다. 포즈/이동과 직교하는 단일 책임 태스크.
 * 틱하지 않으므로 비용이 없다. 각 상태가 자기 인터랙션 가용 여부를 명시하도록 상태마다 둔다(직접 복원 시에도 일관).
 */
USTRUCT(meta = (DisplayName = "Wx Elevator Interaction"))
struct FWxStateTreeTask_ElevatorInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_ElevatorInteractionInstanceData;

	FWxStateTreeTask_ElevatorInteraction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── ElevatorStateIs: 현재 State 검사 ───────────────────────────────────────

USTRUCT()
struct FWxStateTreeCondition_ElevatorStateIsInstanceData
{
	GENERATED_BODY()

	/** 비교할 엘리베이터 상태. 이 조건을 단 상태의 enter condition 으로 사용. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	EWxElevatorState State = EWxElevatorState::DoorsClosed;
};

/**
 * 소유 엘리베이터의 현재 State 가 지정 State 와 같은지 검사한다.
 * 각 비주얼 상태(DoorsClosed/DoorsOpening/DoorsOpen/DoorsClosing/Moving)의 enter condition 으로 사용하여,
 * 시작/복원/재선택 시 현재 State 에 맞는 상태를 선택한다.
 */
USTRUCT(meta = (DisplayName = "Wx Elevator State Is"))
struct FWxStateTreeCondition_ElevatorStateIs : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeCondition_ElevatorStateIsInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
