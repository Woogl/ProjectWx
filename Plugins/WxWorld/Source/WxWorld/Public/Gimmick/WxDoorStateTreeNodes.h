// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxDoor.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "WxDoorStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

/**
 * AWxDoor 의 StateTree 구동용 노드 모음.
 * 상태·전이는 ST_Door 에셋에서 author 하며, 여기의 노드는 소유 AWxDoor 의 얇은 프리미티브
 * (문 포즈/인터랙션 토글/State 조회·승급) 만 호출한다. 컨텍스트 액터는 StateTreeComponentSchema 가
 * 제공하는 소유 액터(= AWxDoor) 이며, 각 노드는 Context.GetOwner() 를 캐스트해 얻는다.
 *
 * 노드는 셋이다(직교 단일 책임):
 *  - DoorPose 는 (bOpen) 으로 현재 포즈에서 목표 포즈로 일정 속도 보간하고, 도달 시 서버가 State 를 승급한다.
 *    시작 시 이미 목표 포즈면(복원/레이트조인) 움직임 없이 스냅, 라이브 전이면 슬라이드. 정지·전이를 한 태스크로 처리.
 *  - DoorInteraction 은 (bEnableInteraction) 으로 콘솔 인터랙션을 토글.
 *  - DoorStateIs 는 (state) 로 각 비주얼 상태의 enter condition 에 재사용.
 */

// ── DoorPose: 현재→목표 보간 포즈 ──────────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_DoorPoseInstanceData
{
	GENERATED_BODY()

	/** 목표 포즈가 열림인지(true=열림 1, false=닫힘 0). 도달 시 승급할 State(Open/Closed)도 이 값이 결정. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bOpen = false;
};

/**
 * Tick 으로 문 개방 알파를 현재값에서 목표(bOpen?1:0)로 DoorAnimDuration 에 맞춘 일정 속도로 보간하고,
 * 목표 도달 시 서버가 State 를 bOpen?Open:Closed 로 승급한다.
 * 시작 시 이미 목표 포즈면(문이 BeginPlay 에서 State 에 맞게 pre-snap) 움직임 없이 즉시 스냅·승급된다.
 */
USTRUCT(meta = (DisplayName = "Wx Door Pose"))
struct FWxStateTreeTask_DoorPose : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_DoorPoseInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── DoorInteraction: 콘솔 인터랙션 토글 ────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_DoorInteractionInstanceData
{
	GENERATED_BODY()

	/** 진입 시 콘솔 인터랙션 활성 여부. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnableInteraction = false;
};

/**
 * 진입 시 콘솔 인터랙션을 지정값으로 토글하고 머문다. 포즈와 직교하는 단일 책임 태스크.
 * 틱하지 않으므로 비용이 없다. 각 상태가 자기 인터랙션 가용 여부를 명시하도록 상태마다 둔다(직접 복원 시에도 일관).
 */
USTRUCT(meta = (DisplayName = "Wx Door Interaction"))
struct FWxStateTreeTask_DoorInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_DoorInteractionInstanceData;

	FWxStateTreeTask_DoorInteraction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── DoorStateIs: 현재 State 검사 ───────────────────────────────────────────

USTRUCT()
struct FWxStateTreeCondition_DoorStateIsInstanceData
{
	GENERATED_BODY()

	/** 비교할 문 상태. 이 조건을 단 상태의 enter condition 으로 사용. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	EWxDoorState State = EWxDoorState::Closed;
};

/**
 * 소유 문의 현재 State 가 지정 State 와 같은지 검사한다.
 * 각 비주얼 상태(Closed/Opening/Open/Closing)의 enter condition 으로 사용하여, 시작/복원/재선택 시 현재 State 에 맞는 상태를 선택한다.
 */
USTRUCT(meta = (DisplayName = "Wx Door State Is"))
struct FWxStateTreeCondition_DoorStateIs : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeCondition_DoorStateIsInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
