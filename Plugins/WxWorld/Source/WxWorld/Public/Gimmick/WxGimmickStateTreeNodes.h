// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxGimmickStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AWxSpawner;
class UAnimSequenceBase;
class UNiagaraSystem;
class USceneComponent;
class USkeletalMeshComponent;
class USoundBase;
class USplineComponent;

/**
 * 모든 Gimmick(AWxGimmick 파생) 의 StateTree 가 공유하는 노드 모음.
 * 여기의 노드는 기믹 종류와 무관한 공통 동작만 다루며, 소유 액터의 얇은 프리미티브만 호출한다.
 * 컨텍스트 액터는 StateTreeComponentSchema 가 제공하는 소유 액터(= AWxGimmick 파생) 이며, 각 노드는 Context.GetOwner() 를 캐스트해 얻는다.
 *
 *  - GimmickInteraction 은 (bEnableInteraction) 으로 기믹의 모든 인터랙션 영역을 일괄 토글한다.
 *  - ComponentMove 는 (TargetComponent, LocalOffset, Duration) 으로 지정 컴포넌트를 아키타입(기준) 포즈에서 기준+offset 으로 일정 속도 슬라이드한다(범용 메시 이동).
 *  - ComponentSplineMove 는 (TargetComponent, Spline, TargetPointIndex, Duration) 으로 지정 컴포넌트를 목표 스플라인 포인트로 옮긴다. 초기 진입이면 목표 포인트로 즉시 스냅, 라이브 전이면 현재(가장 가까운) 포인트에서 목표까지 곡선을 따라 일정 속도 이동한다(State 가 목표 끝점을 직접 선언하므로 복원도 정확).
 *  - PlayAnimation 은 (TargetMesh, Animation) 으로 초기 진입이면 끝 프레임 스냅, 라이브 전이면 처음부터 재생한다. 범용 애니 재생.
 *  - PlayFx 는 (AttachComponent, Niagara, Sound) 로 라이브 전이 진입 시에만 트리거 FX 를 1회 재생한다(복원 시 침묵).
 *  - TriggerSpawners 는 (Spawners) 로 라이브 전이 진입 시 권위 측에서만 각 스포너의 Respawn 을 호출한다(복원 시 재실행 안 함).
 *
 * 초기 진입(StateTree 시작/복원/레이트조인) 과 라이브 전이는 모든 노드가 Transition.SourceStateID 유효성으로 구분한다.
 *
 * 모든 노드는 자기 작업이 끝나면 Succeeded 를 반환한다(즉시형은 진입 직후, ComponentMove/ComponentSplineMove 는 목표 도달 시, PlayAnimation 은 재생 종료 시). 이로써 상태가 스스로 완료돼 OnComplete 전이를 발화시킬 수 있다.
 * 상태가 언제 완료로 판정되는지(완료 판정에 포함할 태스크·All/Any)는 에셋이 상태별로 정한다. 완료 전이가 없는 머무는 상태는 그 태스크를 완료 판정에서 빼야 루트 재선택 thrash 를 피한다.
 */

// ── GimmickInteraction: 인터랙션 일괄 토글 ─────────────────────────────────

USTRUCT()
struct FWxStateTreeTask_GimmickInteractionInstanceData
{
	GENERATED_BODY()

	/** 진입 시 이 기믹의 모든 인터랙션 영역 활성 여부. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnableInteraction = false;
};

/**
 * 진입 시 소유 기믹의 모든 인터랙션 영역(UWxInteractionComponent)을 지정값으로 일괄 토글한 뒤 Succeeded 로 완료한다.
 * 포즈/이동 등과 직교하는 단일 책임 태스크. 틱하지 않으므로 비용이 없다.
 * 각 상태가 자기 인터랙션 가용 여부를 명시하도록 상태마다 둔다(직접 복원 시에도 일관).
 */
USTRUCT(meta = (DisplayName = "Wx Gimmick Interaction"))
struct FWxStateTreeTask_GimmickInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_GimmickInteractionInstanceData;

	FWxStateTreeTask_GimmickInteraction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── ComponentMove: 지정 컴포넌트 직선 슬라이드 ──────────────────────────────

USTRUCT()
struct FWxStateTreeTask_ComponentMoveInstanceData
{
	GENERATED_BODY()

	/** 옮길 씬 컴포넌트. ST 에셋에서 Context 액터의 컴포넌트(예: DoorLeft)로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USceneComponent> TargetComponent;

	/** 기준(아키타입) 포즈 대비 목표 변위(로컬). 각 상태가 자기 목표를 직접 지정한다(머무를 위치는 0). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector LocalOffset = FVector::ZeroVector;

	/** 목표까지 슬라이드 시간(초). 0 이하면 즉시 스냅. 속도는 LocalOffset/Duration 고정값이라 재진입과 무관하게 일정하다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Duration = 1.f;
};

/**
 * 지정 컴포넌트를 기준(아키타입) 상대 위치에서 기준+LocalOffset 으로 일정 속도 슬라이드하고, 도달하면 Succeeded 를 반환해 상태를 완료시킨다.
 * State 를 읽지 않는 순수 비주얼 태스크라 어떤 기믹이든 메시 이동에 재사용한다.
 * 시작 시 이미 목표거나(복원/레이트조인) 초기 진입이거나 길이가 0이면 움직임 없이 즉시 스냅해 곧바로 완료, 라이브 전이면 슬라이드 후 도달 시 완료한다.
 */
USTRUCT(meta = (DisplayName = "Wx Component Move"))
struct FWxStateTreeTask_ComponentMove : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_ComponentMoveInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── ComponentSplineMove: 지정 컴포넌트 스플라인 경로 이동 ─────────────────────

USTRUCT()
struct FWxStateTreeTask_ComponentSplineMoveInstanceData
{
	GENERATED_BODY()

	/** 스플라인 위를 움직일 씬 컴포넌트. ST 에셋에서 Context 액터의 컴포넌트로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USceneComponent> TargetComponent;

	/** 경로를 정의하는 스플라인. 컴포넌트는 이 경로 위를 탄다고 가정한다. ST 에셋에서 Context 액터의 스플라인으로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USplineComponent> Spline;

	/** 이 상태가 목표하는 스플라인 포인트 인덱스. 각 상태가 자기 끝점을 직접 선언한다(초기 진입 스냅·라이브 슬라이드의 목적지). 범위를 벗어나면 클램프. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	int32 TargetPointIndex = 0;

	/** 목표 포인트까지 주파 시간(초). 0 이하면 즉시 스냅. 속도는 호 길이/Duration 고정값이라 재진입과 무관하게 일정하다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Duration = 1.f;

	/** (런타임) Tick 이 보간하는 현재 스플라인 거리. EnterState 에서 시작 포인트 거리로 초기화한다. */
	UPROPERTY()
	float CurrentDistance = 0.f;

	/** (런타임) 목표 포인트(TargetPointIndex)의 스플라인 거리. EnterState 에서 캡처한다. */
	UPROPERTY()
	float TargetDistance = 0.f;

	/** (런타임) 시작→목표 구간의 일정 속도(초당 스플라인 거리). EnterState 에서 1회 산출한다(시작점은 동적이라 Tick 이 고정 저자값에서 재계산 불가). */
	UPROPERTY()
	float MoveSpeed = 0.f;
};

/**
 * 지정 컴포넌트를 TargetPointIndex 가 가리키는 스플라인 포인트로 옮기고, 도달하면 Succeeded 를 반환해 상태를 완료시킨다. 각 상태가 자기 목표 끝점을 직접 선언하는 순수 비주얼 태스크라 어떤 기믹이든 경로 이동에 재사용한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 목표 포인트로 즉시 스냅한다 — State 가 끝점을 직접 가리키므로 복제/복원된 상태의 위치를 정확히 복원한다(C++ 스냅 불필요).
 * 라이브 전이면 현재 위치에서 가장 가까운 포인트를 시작점으로 잡아 목표 포인트까지 곡선을 따라 일정 속도 슬라이드한다(Duration 0 이하·이미 목표면 즉시 스냅). 정지 시 항상 포인트에 머무는 사용을 전제한다.
 */
USTRUCT(meta = (DisplayName = "Wx Component Spline Move"))
struct FWxStateTreeTask_ComponentSplineMove : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_ComponentSplineMoveInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── PlayAnimation: 지정 스켈레탈 메시 애니 재생/스냅 ───────────────────────

USTRUCT()
struct FWxStateTreeTask_PlayAnimationInstanceData
{
	GENERATED_BODY()

	/** 애니메이션을 적용할 스켈레탈 메시. ST 에셋에서 Context 액터의 컴포넌트로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USkeletalMeshComponent> TargetMesh;

	/** 진입 시 적용할 애니메이션. 초기 진입이면 끝 프레임으로 스냅, 라이브 전이면 처음부터 재생한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UAnimSequenceBase> Animation;
};

/**
 * 진입 시 지정 스켈레탈 메시에 애니메이션을 적용하고, 재생이 끝나면 Succeeded 를 반환해 상태를 완료시킨다. State 를 읽지 않는 순수 비주얼 태스크라 어떤 기믹이든 재사용한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 끝 프레임으로 스냅해 발동 완료 포즈를 복원하고 곧바로 완료, 라이브 전이면 처음부터 재생한다.
 * 재생 종료를 감지하려고 틱한다 — 싱글노드 인스턴스가 멈추면 완료로 본다.
 */
USTRUCT(meta = (DisplayName = "Wx Play Animation"))
struct FWxStateTreeTask_PlayAnimation : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlayAnimationInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── PlayFx: 라이브 진입 시 Niagara/사운드 1회 재생 ───────────────────────────

USTRUCT()
struct FWxStateTreeTask_PlayFxInstanceData
{
	GENERATED_BODY()

	/** Niagara 를 attach 할 컴포넌트. ST 에셋에서 Context 액터의 컴포넌트(예: Console)로 바인딩한다. 비우면 액터 위치에 재생. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USceneComponent> AttachComponent;

	/** 라이브 진입 시 재생할 Niagara 시스템. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UNiagaraSystem> Niagara;

	/** 라이브 진입 시 재생할 사운드(액터 위치). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USoundBase> Sound;
};

/**
 * 라이브 전이로 진입할 때 Niagara/사운드를 1회 재생하고 Succeeded 로 완료한다(트리거 FX). State 를 읽지 않아 어떤 기믹이든 재사용한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 재생하지 않는다 — 발동 FX 는 발동 순간에만 울리고 복원 시엔 침묵한다.
 * 모든 피어(서버+클라)가 각자 진입 시 로컬 재생하므로 별도 멀티캐스트가 필요 없다. 틱하지 않으므로 비용이 없다.
 */
USTRUCT(meta = (DisplayName = "Wx Play Fx"))
struct FWxStateTreeTask_PlayFx : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlayFxInstanceData;

	FWxStateTreeTask_PlayFx();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── TriggerSpawners: 라이브 진입 시 권위 측에서 스포너 트리거 ─────────────────

USTRUCT()
struct FWxStateTreeTask_TriggerSpawnersInstanceData
{
	GENERATED_BODY()

	/** 라이브 진입 시 Respawn() 을 호출할 스포너들. ST 에셋에서 Context 액터의 프로퍼티(예: TargetSpawners)로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TArray<TSoftObjectPtr<AWxSpawner>> Spawners;
};

/**
 * 라이브 전이로 진입할 때 권위 측에서만 지정 스포너들의 Respawn() 을 호출하고 Succeeded 로 완료한다(1회성 스폰 트리거).
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 호출하지 않는다 — 스폰은 발동 순간에만 일어나고 복원 시엔 재실행하지 않는다.
 * 스트리밍 아웃된 스포너는 강제 로드하지 않고 스킵한다. 틱하지 않으므로 비용이 없다.
 */
USTRUCT(meta = (DisplayName = "Wx Trigger Spawners"))
struct FWxStateTreeTask_TriggerSpawners : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_TriggerSpawnersInstanceData;

	FWxStateTreeTask_TriggerSpawners();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
