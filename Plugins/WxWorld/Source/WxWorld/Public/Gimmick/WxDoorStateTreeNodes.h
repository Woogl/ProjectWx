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
 * (인터랙션 토글/문 포즈/슬라이드/State 조회·승급) 만 호출한다. 컨텍스트 액터는 StateTreeComponentSchema 가
 * 제공하는 소유 액터(= AWxDoor) 이며, 각 노드는 Context.GetOwner() 를 캐스트해 얻는다.
 *
 * 노드는 방향·State 를 명시한다:
 *  - DoorPose 는 (interaction, bOpen) 로 Closed/Open 정지 상태에 재사용.
 *  - DoorOpening(0→1, →Open) 과 DoorClosing(1→0, →Closed) 은 각 전이 상태의 방향성 슬라이드 애니메이션.
 *  - DoorStateIs 는 (state) 로 각 비주얼 상태의 enter condition 에 재사용.
 */

// ── DoorPose: 정적 포즈 유지 ───────────────────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_DoorPoseInstanceData
{
	GENERATED_BODY()

	/** 진입 시 콘솔 인터랙션 활성 여부. Closed=true, Open=false. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnableInteraction = false;

	/** 진입 시 열린 포즈로 스냅할지. Closed=false, Open=true. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bOpen = false;
};

/**
 * 진입 시 콘솔 인터랙션을 토글하고 문을 지정 알파로 스냅한 뒤, 재선택까지 Running 으로 머무는 포즈 유지 태스크.
 * 틱하지 않으므로 정지 상태의 문은 매 프레임 비용이 없다. Closed(true,false)·Open(false,true) 두 정지 상태에서 재사용.
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

// ── DoorOpening: 개방 슬라이드(0→1, →Open) ─────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_DoorOpeningInstanceData
{
	GENERATED_BODY()

	/** 슬라이드 시작 후 경과 시간(초). */
	UPROPERTY()
	float Elapsed = 0.f;
};

/**
 * 진입 시 인터랙션을 끄고, Tick 으로 개방 알파를 0→1 로 보간한다. 1 도달 시 서버가 Open 으로 승급한다.
 * 애니 길이는 소유 AWxDoor 의 DoorAnimDuration 을 단일 출처로 읽는다.
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

// ── DoorClosing: 닫기 슬라이드(1→0, →Closed) ───────────────────────────────

USTRUCT()
struct FWxStateTreeTask_DoorClosingInstanceData
{
	GENERATED_BODY()

	/** 슬라이드 시작 후 경과 시간(초). */
	UPROPERTY()
	float Elapsed = 0.f;
};

/**
 * 진입 시 인터랙션을 끄고, Tick 으로 개방 알파를 1→0 로 보간한다. 0 도달 시 서버가 Closed 로 승급한다.
 * 애니 길이는 소유 AWxDoor 의 DoorAnimDuration 을 단일 출처로 읽는다.
 */
USTRUCT(meta = (DisplayName = "Wx Door Closing"))
struct FWxStateTreeTask_DoorClosing : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_DoorClosingInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
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
};
