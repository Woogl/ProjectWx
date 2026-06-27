// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "WxGimmickStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AActor;
class ALevelSequenceActor;
class AWxSpawner;
class UAnimSequenceBase;
class ULevelSequence;
class ULevelSequencePlayer;
class UNiagaraSystem;
class USceneComponent;
class USkeletalMeshComponent;
class USoundBase;
class USplineComponent;
class UWxInteractionComponent;

/**
 * 모든 Gimmick(AWxGimmick 파생) 의 StateTree 가 공유하는 노드 모음.
 * 여기의 노드는 기믹 종류와 무관한 공통 동작만 다루며, 소유 액터의 얇은 프리미티브만 호출한다.
 * 컨텍스트 액터는 StateTreeComponentSchema 가 제공하는 소유 액터(= AWxGimmick 파생) 이며, 각 노드는 Context.GetOwner() 를 캐스트해 얻는다.
 *
 *  - EnableInteraction 은 (InteractionComponent, bEnable) 으로 지정 상호작용 컴포넌트 하나의 활성/비활성을 토글한다.
 *  - EnablePlayerInput 은 (bEnable) 으로 로컬 플레이어 폰의 입력 전체를 진입 시 1회 토글한다(컷신 등 연출 중 조작 차단).
 *  - ComponentMove 는 (TargetComponent, LocalOffset, Duration) 으로 지정 컴포넌트를 현재 위치에서 기준(아키타입)+offset 으로 일정 속도 슬라이드한다(범용 메시 이동, 목표=아키타입인 닫기 방향도 지원).
 *  - ComponentSplineMove 는 (TargetComponent, Spline, TargetPointIndex, Duration) 으로 지정 컴포넌트를 목표 스플라인 포인트로 옮긴다. 초기 진입이면 목표 포인트로 즉시 스냅, 라이브 전이면 현재(가장 가까운) 포인트에서 목표까지 곡선을 따라 일정 속도 이동한다(State 가 목표 끝점을 직접 선언하므로 복원도 정확).
 *  - AtSplinePoint(조건) 는 (TargetComponent, Spline, PointIndex, bInvert) 로 대상 컴포넌트의 현재 위치에서 가장 가까운 스플라인 포인트가 PointIndex 와 같은지 검사한다(기하 판정, 멤버 저장 없음). ComponentSplineMove 와 nearest 로직을 공유한다.
 *  - PlayAnimation 은 (TargetMesh, Animation) 으로 초기 진입이면 끝 프레임 스냅, 라이브 전이면 처음부터 재생한다. 범용 애니 재생.
 *  - PlayLevelSequence 는 (LevelSequence) 로 라이브 전이 진입 시 시퀀스를 재생하고 Tick 으로 종료를 폴링하다, 종료 시 시퀀스를 정리하고 권위 측이면 소유 기믹의 HandleLevelSequenceFinished 로 통지한 뒤 Succeeded 를 반환한다(호스트가 State 복귀를 구동; OnComplete 전이를 쓰는 기믹도 그대로 가능). 입력 차단은 별도 EnablePlayerInput 이 맡는다. 중도 이탈 시 ExitState 가 시퀀스 정지·정리(복원 시 침묵·통지 없음).
 *  - PlaySound 는 (Sound) 로 라이브 전이 진입 시에만 사운드를 1회 재생한다(복원 시 침묵).
 *  - SpawnNiagara 는 (AttachComponent, Niagara) 로 라이브 전이 진입 시에만 Niagara 를 1회 재생한다(복원 시 침묵).
 *  - TriggerSpawners 는 (Spawners) 로 라이브 전이 진입 시 권위 측에서만 각 스포너의 Respawn 을 호출한다(복원 시 재실행 안 함).
 *  - SpawnActor 는 (ActorClass, SpawnPoint, Interval, Lifetime, SpawnCollisionHandlingOverride) 로 매 틱 권위 측에서 SpawnPoint(없으면 오너) 트랜스폼에 일정 간격으로 액터를 스폰하고 살아있는 목록을 유지한다(Lifetime 양수면 자동 파괴, 완료 없는 머무는 태스크, 상태 이탈 시 전부 파괴).
 *
 * 초기 진입(StateTree 시작/복원/레이트조인) 과 라이브 전이는 모든 노드가 Transition.SourceStateID 유효성으로 구분한다.
 *
 * 모든 노드는 자기 작업이 끝나면 Succeeded 를 반환한다(즉시형은 진입 직후, ComponentMove/ComponentSplineMove 는 목표 도달 시, PlayAnimation/PlayLevelSequence 는 재생 종료 시). 이로써 상태가 스스로 완료돼 OnComplete 전이를 발화시킬 수 있다.
 * 상태가 언제 완료로 판정되는지(완료 판정에 포함할 태스크·All/Any)는 에셋이 상태별로 정한다. 완료 전이가 없는 머무는 상태는 그 태스크를 완료 판정에서 빼야 루트 재선택 thrash 를 피한다.
 */

// ── EnableInteraction: 지정 상호작용 컴포넌트 토글 ─────────────────────────────

USTRUCT()
struct FWxStateTreeTask_EnableInteractionInstanceData
{
	GENERATED_BODY()

	/** 토글할 상호작용 컴포넌트. ST 에셋에서 Context 액터의 컴포넌트(예: ConsoleInteraction)로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	/** 진입 시 위 컴포넌트의 상호작용 활성 여부. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnable = false;
};

/**
 * 진입 시 지정 상호작용 컴포넌트(UWxInteractionComponent) 하나의 활성/비활성을 bEnable 로 토글한 뒤 Succeeded 로 완료한다.
 * 포즈/이동 등과 직교하는 단일 책임 태스크. 인터랙션이 여러 개인 기믹은 영역마다 노드를 둔다. 틱하지 않으므로 비용이 없다.
 * 각 상태가 자기 인터랙션 가용 여부를 명시하도록 상태마다 둔다(직접 복원 시에도 일관). 컴포넌트가 비면 Failed.
 * 순간 side-effect 라 기본적으로 상태 완료를 구동하지 않는다(bConsideredForCompletion=false; 토글만 든 정지 leaf 가 즉시 완료→재선택 루프에 빠지지 않도록). 인스턴스별로 다시 켤 수 있다.
 */
USTRUCT(meta = (DisplayName = "Wx Enable Interaction"))
struct FWxStateTreeTask_EnableInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_EnableInteractionInstanceData;

	FWxStateTreeTask_EnableInteraction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── EnablePlayerInput: 로컬 플레이어 폰 입력 토글 ─────────────────────────────

USTRUCT()
struct FWxStateTreeTask_EnablePlayerInputInstanceData
{
	GENERATED_BODY()

	/** 진입 시 로컬 플레이어 폰의 입력 활성 여부. false 면 컷신 등 연출 중 조작을 막는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnable = true;
};

/**
 * 진입 시 로컬 플레이어 폰의 입력 전체를 bEnable 로 토글한 뒤 Succeeded 로 완료한다(EnableInteraction 과 동형의 토글 태스크).
 * 각 상태가 자기 입력 가용 여부를 선언하도록 상태마다 둔다(예: 컷신 Playing 은 false, Idle 은 true). 직접 복원/레이트조인 시에도 일관되게 적용된다.
 * 로컬 플레이어 컨트롤러/폰이 없으면(예: 데디 서버) 노옵. 틱하지 않으므로 비용이 없다.
 * 순간 side-effect 라 기본적으로 상태 완료를 구동하지 않는다(bConsideredForCompletion=false; 컷신은 PlayLevelSequence 가 완료를 구동). 인스턴스별로 다시 켤 수 있다.
 */
USTRUCT(meta = (DisplayName = "Wx Enable Player Input"))
struct FWxStateTreeTask_EnablePlayerInput : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_EnablePlayerInputInstanceData;

	FWxStateTreeTask_EnablePlayerInput();

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

	/** 목표까지 슬라이드 시간(초). 0 이하면 즉시 스냅. 속도는 시작→목표 실제 거리/Duration 으로 EnterState 에서 1회 산출한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Duration = 1.f;

	/** (런타임) 시작→목표 구간의 일정 속도(초당 로컬 거리). EnterState 에서 1회 산출한다(LocalOffset 크기가 아니라 실제 시작 위치 기준 거리라, 목표가 아키타입인 닫기도 0 이 아니다). */
	UPROPERTY()
	float MoveSpeed = 0.f;
};

/**
 * 지정 컴포넌트를 현재 상대 위치에서 기준(아키타입)+LocalOffset 으로 일정 속도 슬라이드하고, 도달하면 Succeeded 를 반환해 상태를 완료시킨다.
 * State 를 읽지 않는 순수 비주얼 태스크라 어떤 기믹이든 메시 이동에 재사용한다.
 * 속도는 시작→목표 실제 거리/Duration 으로 EnterState 에서 1회 산출하므로, 목표가 아키타입(offset 0)인 '닫기' 방향도 일정 속도로 슬라이드한다.
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

// ── GimmickStateIs: 기믹의 현재 State 태그가 지정 태그와 같은지 검사(조건) ─────────

USTRUCT()
struct FWxStateTreeCondition_GimmickStateIsInstanceData
{
	GENERATED_BODY()

	/** 비교할 State 태그. 이 상태가 어느 Gimmick.* State 일 때 진입할지 author 한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "Gimmick"))
	FGameplayTag State;

	/** 결과를 반전(현재 State 가 위 태그가 "아닐" 때 참). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;
};

/**
 * 소유 기믹(AWxGimmick)의 현재 권위 State 태그가 지정 State 와 정확히 같으면 참을 반환한다(bInvert 면 반전).
 * 전 상태를 동일한 enter 조건으로 게이트하는 용도 — 완료 후 Root 재선택이 현재 State 와 일치하는 상태(=자기 자신)로 돌아오게 해 정지 상태가 무해하게 머문다.
 * State 를 읽기만 하며 액터 프로퍼티 바인딩이 불필요하다(Context.GetOwner() 캐스트로 직접 조회).
 */
USTRUCT(meta = (DisplayName = "Wx Gimmick State Is"))
struct FWxStateTreeCondition_GimmickStateIs : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeCondition_GimmickStateIsInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

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

// ── PlayLevelSequence: 라이브 진입 시 Level Sequence 재생, 종료 시 완료 ──

USTRUCT()
struct FWxStateTreeTask_PlayLevelSequenceInstanceData
{
	GENERATED_BODY()

	/** 재생할 Level Sequence. ST 에셋에서 Context 액터의 프로퍼티(예: LevelSequence)로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<ULevelSequence> LevelSequence;

	/** (런타임) 생성한 시퀀스 플레이어. 정리 시 정지·해제한다. */
	UPROPERTY()
	TObjectPtr<ULevelSequencePlayer> Player;

	/** (런타임) CreateLevelSequencePlayer 가 만든 액터. 정리 시 Destroy 한다. */
	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;
};

/**
 * 라이브 전이로 진입할 때 Level Sequence 를 재생하고, 재생이 끝나면 소유 기믹에 통지한 뒤 Succeeded 를 반환한다. State 를 읽지 않아 어떤 기믹이든 재사용한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 재생·통지 없이 곧바로 완료한다 — 컷신은 발동 순간에만 재생하고 복원 시엔 침묵한다. 라이브 진입인데 재생할 게 없으면(시퀀스/월드 부재·플레이어 생성 실패) 호스트가 갇히지 않게 곧장 통지하고 완료한다.
 * 입력 차단은 직교 태스크(EnablePlayerInput)가 맡고, 이 노드는 재생만 다룬다.
 * Tick 이 ULevelSequencePlayer::IsPlaying 로 종료를 폴링하다, 종료되면 시퀀스를 정리하고 권위 측이면 소유 기믹의 HandleLevelSequenceFinished 로 통지한다 — 호스트가 그 통지로 권위 State 전이(예: Idle 복귀)를 구동한다(OnComplete 전이를 쓰는 기믹도 그대로 가능). OnFinished 콜백 중 시퀀스 액터 파괴를 피하려고 폴링→다음 틱 정리를 쓴다.
 * 중도 이탈·액터 파괴 시엔 ExitState 가 시퀀스를 정지·정리한다(멱등, 통지 없음). 모든 피어가 각자 진입 시 로컬 재생하므로 별도 멀티캐스트가 필요 없다.
 */
USTRUCT(meta = (DisplayName = "Wx Play Level Sequence"))
struct FWxStateTreeTask_PlayLevelSequence : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlayLevelSequenceInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── PlaySound: 라이브 진입 시 사운드 1회 재생 ───────────────────────────────

USTRUCT()
struct FWxStateTreeTask_PlaySoundInstanceData
{
	GENERATED_BODY()

	/** 라이브 진입 시 재생할 사운드(액터 위치). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USoundBase> Sound;
};

/**
 * 라이브 전이로 진입할 때 사운드를 액터 위치에서 1회 재생하고 Succeeded 로 완료한다(트리거 사운드). State 를 읽지 않아 어떤 기믹이든 재사용한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 재생하지 않는다 — 발동 사운드는 발동 순간에만 울리고 복원 시엔 침묵한다.
 * 모든 피어(서버+클라)가 각자 진입 시 로컬 재생하므로 별도 멀티캐스트가 필요 없다. 틱하지 않으므로 비용이 없다.
 */
USTRUCT(meta = (DisplayName = "Wx Play Sound"))
struct FWxStateTreeTask_PlaySound : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlaySoundInstanceData;

	FWxStateTreeTask_PlaySound();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ── SpawnNiagara: 라이브 진입 시 Niagara 1회 재생 ───────────────────────────

USTRUCT()
struct FWxStateTreeTask_SpawnNiagaraInstanceData
{
	GENERATED_BODY()

	/** Niagara 를 attach 할 컴포넌트. ST 에셋에서 Context 액터의 컴포넌트(예: Console)로 바인딩한다. 비우면 액터 위치에 재생. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USceneComponent> AttachComponent;

	/** 라이브 진입 시 재생할 Niagara 시스템. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UNiagaraSystem> Niagara;
};

/**
 * 라이브 전이로 진입할 때 Niagara 를 1회 재생하고 Succeeded 로 완료한다(트리거 FX). State 를 읽지 않아 어떤 기믹이든 재사용한다.
 * AttachComponent 가 있으면 그 컴포넌트에 붙여 재생하고, 비우면 액터 위치에 재생한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 재생하지 않는다 — 발동 FX 는 발동 순간에만 울리고 복원 시엔 침묵한다.
 * 모든 피어(서버+클라)가 각자 진입 시 로컬 재생하므로 별도 멀티캐스트가 필요 없다. 틱하지 않으므로 비용이 없다.
 */
USTRUCT(meta = (DisplayName = "Wx Spawn Niagara"))
struct FWxStateTreeTask_SpawnNiagara : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SpawnNiagaraInstanceData;

	FWxStateTreeTask_SpawnNiagara();

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

// ── SpawnActor: SpawnPoint 트랜스폼에 일정 간격으로 액터 스폰 ───────────────────

USTRUCT()
struct FWxStateTreeTask_SpawnActorInstanceData
{
	GENERATED_BODY()

	/** 스폰할 액터 클래스. ST 에셋에서 직접 지정한다(레이저 벽 BP 등 클래스 레벨 에셋). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSubclassOf<AActor> ActorClass;

	/** 스폰 기준이 되는 씬 컴포넌트. 그 월드 트랜스폼(위치·회전·스케일)에 스폰한다. ST 에셋에서 Context 액터의 컴포넌트(예: SpawnPoint)로 바인딩한다. 비우면 오너 액터 트랜스폼에 스폰. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USceneComponent> SpawnPoint;

	/** 스폰 간격(초). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.05"))
	float Interval = 2.f;

	/** 스폰체 수명(초). 0 이하면 무한(직접 파괴/이탈 정리에 맡김). 양수면 스폰 시 SetLifeSpan 으로 자동 파괴된다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Lifetime = 0.f;

	/** 스폰 시 충돌 처리 방식(FActorSpawnParameters 의 SpawnCollisionHandlingOverride 로 전달). 기본은 위치 보정 없이 항상 스폰. 겹침을 피하거나 보정하려면 디자이너가 바꾼다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	/** (런타임) 살아있는 스폰체 목록. 후속 이동 노드가 이 프로퍼티를 바인딩 소스로 읽어 이동시킬 수 있다. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	/** (런타임) 마지막 스폰 이후 누적 시간. Interval 도달 시 1회 스폰하고 차감한다. */
	UPROPERTY()
	float TimeSinceLastSpawn = 0.f;
};

/**
 * 매 틱 권위 측에서 SpawnPoint(없으면 오너) 의 월드 트랜스폼에 ActorClass 를 Interval 마다 1회 스폰하고 살아있는 목록(SpawnedActors)을 유지한다. State 를 읽지 않아 어떤 기믹이든 주기 스폰에 재사용한다(예: LaserCorridor 의 레이저 벽).
 * 스폰 위치·회전·크기는 SpawnPoint 트랜스폼이 그대로 정하고(스폰체 크기는 SpawnPoint 스케일), 수명은 Lifetime 으로 받아 양수면 SetLifeSpan 으로 자동 파괴한다. 스폰 충돌 처리는 SpawnCollisionHandlingOverride 로 디자이너가 정한다. 후속 이동이 필요하면 이동 노드가 SpawnedActors 를 바인딩해 구동하므로, 에셋에서 이 노드를 그 앞에 둔다.
 * 스폰은 서버 권위 사건이라 권위 측에서만 일어나고(클라는 복제 추종), 스폰체는 Transient 라 복원할 포즈가 없어 초기 진입·라이브 구분 없이 진입 즉시 스폰을 재개한다.
 * 완료 전이가 없는 머무는 태스크라 항상 Running 을 유지하며(이 태스크는 상태 완료 판정에서 빼야 한다), 상태를 떠날 때 ExitState 가 남은 스폰체를 전부 파괴한다.
 */
USTRUCT(meta = (DisplayName = "Wx Spawn Actor"))
struct FWxStateTreeTask_SpawnActor : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SpawnActorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
