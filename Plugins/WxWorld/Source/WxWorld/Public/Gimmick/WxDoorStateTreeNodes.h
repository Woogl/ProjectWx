// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "WxDoorStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

/**
 * AWxDoor 의 StateTree 구동용 노드 모음.
 * 상태·전이는 ST_Door 에셋에서 author 하며, 여기의 노드는 소유 AWxDoor 의 얇은 프리미티브
 * (인터랙션 토글/문 포즈/발동 여부) 만 호출한다. 컨텍스트 액터는 StateTreeComponentSchema 가
 * 제공하는 소유 액터(= AWxDoor) 이며, 각 노드는 Context.GetOwner() 를 캐스트해 얻는다.
 */

// ── DoorPose: Closed/Open 정적 포즈 유지 ───────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_DoorPoseInstanceData
{
	GENERATED_BODY()

	/** 진입 시 콘솔 인터랙션 활성 여부. Closed=true, Open=false. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnableInteraction = false;

	/** 진입 시 문 개방 알파(0=닫힘, 1=열림). Closed=0, Open=1. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float OpenAlpha = 0.f;
};

/**
 * 진입 시 콘솔 인터랙션을 토글하고 문을 지정 알파로 스냅한 뒤, 전이(이벤트/완료)까지 Running 으로 머무는 포즈 유지 태스크.
 * 틱하지 않으므로 닫힌/열린 문은 매 프레임 비용이 없다. Closed(true,0)·Open(false,1) 두 상태에서 재사용.
 */
USTRUCT(meta = (DisplayName = "Wx Door Pose"))
struct FWxStateTreeTask_DoorPose : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_DoorPoseInstanceData;

	FWxStateTreeTask_DoorPose();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ── DoorOpening: 개방 애니메이션 ───────────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_DoorOpeningInstanceData
{
	GENERATED_BODY()

	/** 개방 시작 후 경과 시간(초). */
	UPROPERTY()
	float Elapsed = 0.f;
};

/**
 * 진입 시 인터랙션을 끄고, Tick 으로 개방 알파를 0→1 보간한다. 1 도달 시 Succeeded.
 * 문 애니 길이는 소유 AWxDoor 의 DoorAnimDuration 을 단일 출처로 읽는다.
 */
USTRUCT(meta = (DisplayName = "Wx Door Opening"))
struct FWxStateTreeTask_DoorOpening : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_DoorOpeningInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// ── DoorTriggered: 발동 여부 조건 ──────────────────────────────────────────

USTRUCT()
struct FWxStateTreeCondition_DoorTriggeredInstanceData
{
	GENERATED_BODY()
};

/**
 * 문이 이미 열림 발동(bTriggered)되었는지 검사한다.
 * Open 상태의 진입 조건으로 사용하여, 복원/스트리밍/레이트조인 등 시작 시점에 이미 열린 문을 애니 없이 Open 으로 초기 선택한다.
 */
USTRUCT(meta = (DisplayName = "Wx Door Triggered"))
struct FWxStateTreeCondition_DoorTriggered : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeCondition_DoorTriggeredInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
