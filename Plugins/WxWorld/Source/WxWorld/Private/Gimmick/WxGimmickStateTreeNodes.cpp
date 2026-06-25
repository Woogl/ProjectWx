// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmickStateTreeNodes.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Gimmick/WxGimmick.h"
#include "Interaction/WxInteractionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "NiagaraFunctionLibrary.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"

namespace
{
	// 로컬 플레이어 폰의 입력 전체를 토글한다. 컷신 등 연출 중 조작을 막고 복구하는 용도. PC/Pawn 이 없으면(예: 데디 서버) 노옵.
	void SetLocalPlayerInputEnabled(UWorld* World, bool bEnabled)
	{
		APlayerController* PC = GEngine ? GEngine->GetFirstLocalPlayerController(World) : nullptr;
		if (!PC)
		{
			return;
		}

		APawn* Pawn = PC->GetPawn();
		if (!Pawn)
		{
			return;
		}

		if (bEnabled)
		{
			Pawn->EnableInput(PC);
		}
		else
		{
			Pawn->DisableInput(PC);
		}
	}

	// 기준 포즈 = 컴포넌트 아키타입(BP/CDO 오서링)의 상대 위치. 런타임 위치가 어디든 안정적 앵커.
	FVector GetMoveAnchor(const USceneComponent* Component)
	{
		const USceneComponent* Archetype = Cast<USceneComponent>(Component->GetArchetype());
		return Archetype ? Archetype->GetRelativeLocation() : Component->GetRelativeLocation();
	}
}

FWxStateTreeTask_EnableInteraction::FWxStateTreeTask_EnableInteraction()
{
	// 인터랙션을 진입 시 1회 토글만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_EnableInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UWxInteractionComponent* Interaction = Instance.InteractionComponent;
	if (!Interaction)
	{
		return EStateTreeRunStatus::Failed;
	}

	Interaction->SetInteractionEnabled(Instance.bEnable);

	// 토글은 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_EnableInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Enable Interaction ({0})"), InstanceData->bEnable);
}
#endif

// ── EnablePlayerInput ─────────────────────────────────────────────────────────

FWxStateTreeTask_EnablePlayerInput::FWxStateTreeTask_EnablePlayerInput()
{
	// 입력을 진입 시 1회 토글만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_EnablePlayerInput::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	SetLocalPlayerInputEnabled(Owner ? Owner->GetWorld() : nullptr, Instance.bEnable);

	// 토글은 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_EnablePlayerInput::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Enable Player Input ({0})"), InstanceData->bEnable);
}
#endif

// ── ComponentMove ───────────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_ComponentMove::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USceneComponent* Component = Instance.TargetComponent;
	if (!Component)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FVector Target = GetMoveAnchor(Component) + Instance.LocalOffset;

	// 초기 진입(StateTree 시작/복원: SourceStateID 무효)·길이 0·이미 목표면 애니 없이 즉시 스냅하고 곧바로 완료한다.
	// 라이브 전이면 Tick 이 슬라이드하다 도달 시 완료한다. 속도 계산은 Tick 이 고정값으로 하므로 여기선 따로 잡지 않는다.
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	const bool bReachNow = bInitialEntry || Instance.Duration <= 0.f || Component->GetRelativeLocation().Equals(Target);
	if (bReachNow)
	{
		Component->SetRelativeLocation(Target);
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_ComponentMove::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USceneComponent* Component = Instance.TargetComponent;
	if (!Component)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FVector Target = GetMoveAnchor(Component) + Instance.LocalOffset;
	FVector NewLocation = Component->GetRelativeLocation();

	// 도달 전까지 일정 속도로 슬라이드한다. 속도는 LocalOffset/Duration 고정값이라
	// 재진입해도 줄어든 거리로 재계산하지 않아 감속 없이 일정하다.
	if (!NewLocation.Equals(Target))
	{
		const float Speed = Instance.Duration > 0.f ? Instance.LocalOffset.Size() / Instance.Duration : Instance.LocalOffset.Size();
		NewLocation = FMath::VInterpConstantTo(NewLocation, Target, DeltaTime, Speed);
		Component->SetRelativeLocation(NewLocation);
	}

	// 도달하면 상태를 완료시킨다.
	return NewLocation.Equals(Target) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ComponentMove::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 움직일 컴포넌트는 보통 바인딩이라 런타임 포인터가 비어 있다. 바인딩 소스명을 우선 보이고, 직접 지정 시 그 이름으로 폴백.
	FText ComponentText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, TargetComponent)), Formatting);
	if (ComponentText.IsEmpty())
	{
		ComponentText = InstanceData->TargetComponent ? FText::FromString(InstanceData->TargetComponent->GetName()) : INVTEXT("(none)");
	}

	return FText::Format(INVTEXT("Wx Component Move ({0})"), ComponentText);
}
#endif

// ── ComponentSplineMove ──────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_ComponentSplineMove::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USceneComponent* Component = Instance.TargetComponent;
	const USplineComponent* Spline = Instance.Spline;
	if (!Component || !Spline)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 포인트가 없으면 목표할 위치가 없으므로 할 일 없이 곧바로 완료한다.
	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	if (NumPoints == 0)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 이 상태가 선언한 목표 포인트의 거리. State 가 끝점을 직접 가리키므로 초기 진입에서도 목적지를 안다.
	const int32 TargetIndex = FMath::Clamp(Instance.TargetPointIndex, 0, NumPoints - 1);
	const float TargetDistance = Spline->GetDistanceAlongSplineAtSplinePoint(TargetIndex);
	Instance.TargetDistance = TargetDistance;

	// 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 목표 포인트로 즉시 스냅한다(복제/복원된 State 의 끝점을 정확히 복원).
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		Instance.CurrentDistance = TargetDistance;
		Instance.MoveSpeed = 0.f;
		Component->SetWorldLocation(Spline->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World));
		// 목표 포인트로 스냅했으니 곧바로 완료한다(복원 시 단계 캐스케이드).
		return EStateTreeRunStatus::Succeeded;
	}

	// 라이브 전이: 현재 위치에서 가장 가까운 스플라인 포인트(vertex)를 시작점으로 잡는다(정지 시 항상 끝점에 주차됨). 거리 비교는 World 공간(부모 관계 무가정).
	const FVector CurrentLocation = Component->GetComponentLocation();
	int32 NearestIndex = 0;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		const float DistSq = FVector::DistSquared(CurrentLocation, Spline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World));
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestIndex = Index;
		}
	}

	const float StartDistance = Spline->GetDistanceAlongSplineAtSplinePoint(NearestIndex);

	// 속도는 시작→목표 호 길이/Duration 고정값이라 재진입해도 일정하다(Duration 0 이하면 아래에서 즉시 스냅).
	const float SegmentLength = FMath::Abs(TargetDistance - StartDistance);
	Instance.MoveSpeed = Instance.Duration > 0.f ? SegmentLength / Instance.Duration : SegmentLength;

	// 길이 0·이미 목표면 즉시 목표로 스냅, 아니면 시작에서 Tick 이 슬라이드한다.
	const bool bSnap = Instance.Duration <= 0.f || FMath::IsNearlyEqual(StartDistance, TargetDistance);
	Instance.CurrentDistance = bSnap ? TargetDistance : StartDistance;
	Component->SetWorldLocation(Spline->GetLocationAtDistanceAlongSpline(Instance.CurrentDistance, ESplineCoordinateSpace::World));

	// 스냅으로 이미 도달했으면 곧바로 완료, 아니면 Tick 이 슬라이드하다 도달 시 완료한다.
	if (bSnap)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_ComponentSplineMove::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USceneComponent* Component = Instance.TargetComponent;
	const USplineComponent* Spline = Instance.Spline;
	if (!Component || !Spline)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 도달 전까지 일정 속도로 스플라인 거리를 보간해 곡선을 따라 이동한다.
	if (!FMath::IsNearlyEqual(Instance.CurrentDistance, Instance.TargetDistance))
	{
		Instance.CurrentDistance = FMath::FInterpConstantTo(Instance.CurrentDistance, Instance.TargetDistance, DeltaTime, Instance.MoveSpeed);
		Component->SetWorldLocation(Spline->GetLocationAtDistanceAlongSpline(Instance.CurrentDistance, ESplineCoordinateSpace::World));
	}

	// 도달하면 상태를 완료시킨다.
	const bool bReached = FMath::IsNearlyEqual(Instance.CurrentDistance, Instance.TargetDistance);
	return bReached ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ComponentSplineMove::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 스플라인은 보통 바인딩이라 런타임 포인터가 비어 있다. 바인딩 소스명을 우선 보이고, 직접 지정 시 그 이름으로 폴백.
	FText SplineText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, Spline)), Formatting);
	if (SplineText.IsEmpty())
	{
		SplineText = InstanceData->Spline ? FText::FromString(InstanceData->Spline->GetName()) : INVTEXT("(none)");
	}

	return FText::Format(INVTEXT("Wx Component Spline Move ({0} → point {1})"), SplineText, FText::AsNumber(InstanceData->TargetPointIndex));
}
#endif

// ── PlayAnimation ──────────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_PlayAnimation::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USkeletalMeshComponent* Mesh = Instance.TargetMesh;
	if (!Mesh || !Instance.Animation)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 끝 프레임으로 스냅해 발동 완료 포즈를 복원하고 곧바로 완료한다.
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		Mesh->SetAnimation(Instance.Animation);
		Mesh->SetPosition(Instance.Animation->GetPlayLength(), false);
		return EStateTreeRunStatus::Succeeded;
	}

	// 라이브 전이면 처음부터 재생하고 Tick 이 종료를 감지해 완료한다.
	Mesh->PlayAnimation(Instance.Animation, false);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayAnimation::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USkeletalMeshComponent* Mesh = Instance.TargetMesh;
	if (!Mesh || !Instance.Animation)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 논루프 재생이 끝나면 싱글노드가 멈추므로(또는 애니가 교체되면) 그 시점에 완료한다.
	const UAnimSingleNodeInstance* SingleNode = Mesh->GetSingleNodeInstance();
	const bool bStillPlaying = SingleNode && SingleNode->GetAnimationAsset() == Instance.Animation && SingleNode->IsPlaying();
	return bStillPlaying ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayAnimation::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Play Animation ({0})"),
		InstanceData->Animation ? FText::FromString(InstanceData->Animation->GetName()) : INVTEXT("(none)"));
}
#endif

// ── PlayLevelSequence ─────────────────────────────────────────────────────────

namespace
{
	// 시퀀스 재생을 정지·정리한다: 플레이어를 멈추고 생성한 시퀀스 액터를 파괴한 뒤 런타임 핸들을 비운다.
	// Tick 의 종료 처리와 상태 이탈·액터 파괴 시 호출되며, 핸들이 비어 있으면 멱등하게 노옵이 된다.
	void FinishSequencePlayback(FWxStateTreeTask_PlayLevelSequenceInstanceData& Instance)
	{
		if (Instance.Player)
		{
			Instance.Player->Stop();
		}

		if (Instance.SequenceActor)
		{
			Instance.SequenceActor->Destroy();
		}

		Instance.SequenceActor = nullptr;
		Instance.Player = nullptr;
	}
}

EStateTreeRunStatus FWxStateTreeTask_PlayLevelSequence::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이거나 시퀀스가 없으면 재생하지 않고 곧바로 완료한다(복원 시 침묵).
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (bInitialEntry || !Instance.LevelSequence || !World)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* NewSequenceActor = nullptr;
	Instance.Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Instance.LevelSequence, PlaybackSettings, NewSequenceActor);
	Instance.SequenceActor = NewSequenceActor;

	// 플레이어 생성 실패면 할 일이 없으니 정리하고 완료한다.
	if (!Instance.Player)
	{
		FinishSequencePlayback(Instance);
		return EStateTreeRunStatus::Succeeded;
	}

	Instance.Player->Play();

	// 상태를 떠날 때까지 머문다. 재생 종료를 Tick 이 폴링해 호스트에 통지하고, State 가 바뀌면 ExitState 가 정리한다.
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayLevelSequence::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 재생 중이면 계속 머문다.
	if (Instance.Player && Instance.Player->IsPlaying())
	{
		return EStateTreeRunStatus::Running;
	}

	// 재생이 끝났다(또는 플레이어 미생성). 시퀀스를 정리하고 완료해, 소유 상태의 OnComplete 전이가 상태를 진행시키게 한다.
	FinishSequencePlayback(Instance);
	return EStateTreeRunStatus::Succeeded;
}

void FWxStateTreeTask_PlayLevelSequence::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 상태 이탈(호스트의 State 복귀)·중도 이탈·액터 파괴 시 시퀀스를 정지·정리한다(멱등).
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	FinishSequencePlayback(Instance);
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayLevelSequence::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Play Level Sequence ({0})"),
		InstanceData->LevelSequence ? FText::FromString(InstanceData->LevelSequence->GetName()) : INVTEXT("(none)"));
}
#endif

// ── PlaySound ─────────────────────────────────────────────────────────────────

FWxStateTreeTask_PlaySound::FWxStateTreeTask_PlaySound()
{
	// 진입 시 1회 재생만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_PlaySound::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 사운드를 재생하지 않고 곧바로 완료한다(발동 순간에만 울림).
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Instance.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(Owner, Instance.Sound, Owner->GetActorLocation());
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlaySound::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Play Sound ({0})"),
		InstanceData->Sound ? FText::FromString(InstanceData->Sound->GetName()) : INVTEXT("(none)"));
}
#endif

// ── SpawnNiagara ──────────────────────────────────────────────────────────────

FWxStateTreeTask_SpawnNiagara::FWxStateTreeTask_SpawnNiagara()
{
	// 진입 시 1회 재생만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_SpawnNiagara::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 트리거 FX 를 재생하지 않고 곧바로 완료한다(발동 순간에만 울림).
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Instance.Niagara)
	{
		// attach 대상이 있으면 그 컴포넌트에 붙여 재생, 없으면 액터 위치에 재생.
		if (Instance.AttachComponent)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(Instance.Niagara, Instance.AttachComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		}
		else
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(Owner, Instance.Niagara, Owner->GetActorLocation());
		}
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SpawnNiagara::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Spawn Niagara ({0})"),
		InstanceData->Niagara ? FText::FromString(InstanceData->Niagara->GetName()) : INVTEXT("(none)"));
}
#endif

// ── TriggerSpawners ───────────────────────────────────────────────────────────

FWxStateTreeTask_TriggerSpawners::FWxStateTreeTask_TriggerSpawners()
{
	// 진입 시 1회 트리거만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_TriggerSpawners::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 스폰을 재실행하지 않고 곧바로 완료한다(발동 순간에만 스폰).
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 스폰은 서버 권위 사건이라 클라 진입은 노옵.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	for (const TSoftObjectPtr<AWxSpawner>& SoftSpawner : Instance.Spawners)
	{
		// 스트리밍 아웃된 스포너는 강제 로드하지 않고 스킵. 디자이너가 콘솔과 같은 영역에 배치되도록 보장해야 함.
		if (AWxSpawner* Spawner = SoftSpawner.Get())
		{
			Spawner->Respawn();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Wx Trigger Spawners: TargetSpawner is null or not loaded."));
		}
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_TriggerSpawners::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Trigger Spawners ({0})"), FText::AsNumber(InstanceData->Spawners.Num()));
}
#endif

// ── SetState ──────────────────────────────────────────────────────────────────

FWxStateTreeTask_SetState::FWxStateTreeTask_SetState()
{
	// 진입 시 1회 쓰기만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_SetState::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 쓰지 않고 곧바로 완료한다 — 저장/복제된 State 를 덮어쓰지 않도록 침묵.
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// State 쓰기는 서버 권위 사건이라 클라 진입은 노옵(클라는 복제 State 를 Enum Compare 전이로 추종). CommitGimmickState 가 권위 가드를 다시 적용한다.
	AWxGimmick* Owner = Cast<AWxGimmick>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Owner->CommitGimmickState(Instance.NewState);

	// 쓰기는 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SetState::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Set State ({0})"), FText::AsNumber(InstanceData->NewState));
}
#endif

// ── LaserSpawn ────────────────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_LaserSpawn::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 스폰체는 Transient 라 복원할 포즈가 없다. 누적기를 간격으로 채워 진입 첫 틱에 즉시 1회 스폰하고, 이후 간격마다 스폰한다.
	Instance.TimeSinceLastSpawn = Instance.Interval;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_LaserSpawn::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// 스폰은 서버 권위 사건이다. 클라는 복제로 스폰체를 추종하므로 진행시키지 않는다.
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Running;
	}

	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	Instance.TimeSinceLastSpawn += DeltaTime;
	if (Instance.TimeSinceLastSpawn < Instance.Interval)
	{
		// 아직 간격에 못 미쳤다. 머무는 태스크라 완료하지 않는다.
		return EStateTreeRunStatus::Running;
	}
	Instance.TimeSinceLastSpawn -= Instance.Interval;

	UWorld* World = Owner->GetWorld();
	const UBoxComponent* Volume = Instance.SpawnVolume;
	if (!World || !Volume || !Instance.ActorClass || Instance.MoveSpeed <= 0.f)
	{
		return EStateTreeRunStatus::Running;
	}

	// 박스 -X 끝에서 스폰하고 +X 가 전진 방향. 수명은 박스를 한 번 주파할 시간이라 +X 끝에 닿을 즈음 자동 파괴된다.
	const FVector Forward = Volume->GetForwardVector();
	const FVector Extent = Volume->GetScaledBoxExtent();
	const FVector Center = Volume->GetComponentLocation();
	const FVector StartLocation = Center - Forward * Extent.X;
	const float Lifetime = (Extent.X * 2.f) / Instance.MoveSpeed;

	// 스폰체 BP 디폴트 YZ extent 를 기준으로, 통로의 scaled YZ extent 에 맞는 스케일을 계산해 SpawnTransform 에 반영.
	FVector SpawnScale(1.f, 1.f, 1.f);
	if (const AActor* ClassCDO = Instance.ActorClass->GetDefaultObject<AActor>())
	{
		if (const UBoxComponent* DefaultBox = ClassCDO->FindComponentByClass<UBoxComponent>())
		{
			const FVector DefaultExtent = DefaultBox->GetUnscaledBoxExtent();
			if (DefaultExtent.Y > KINDA_SMALL_NUMBER && DefaultExtent.Z > KINDA_SMALL_NUMBER)
			{
				SpawnScale.Y = Extent.Y / DefaultExtent.Y;
				SpawnScale.Z = Extent.Z / DefaultExtent.Z;
			}
		}
	}

	const FTransform SpawnTransform(Volume->GetComponentRotation(), StartLocation, SpawnScale);
	AActor* Spawned = World->SpawnActorDeferred<AActor>(Instance.ActorClass, SpawnTransform, Owner, Owner->GetInstigator(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Spawned)
	{
		return EStateTreeRunStatus::Running;
	}
	Spawned->SetLifeSpan(Lifetime);
	Spawned->FinishSpawning(SpawnTransform);

	// 자동 파괴(수명 만료)된 항목은 pending-kill 이라 IsValid=false 로 걸러 목록을 유계로 유지한다.
	for (int32 Index = Instance.SpawnedActors.Num() - 1; Index >= 0; --Index)
	{
		if (!IsValid(Instance.SpawnedActors[Index]))
		{
			Instance.SpawnedActors.RemoveAtSwap(Index);
		}
	}
	Instance.SpawnedActors.Add(Spawned);

	// 완료 전이 없는 머무는 상태다. 항상 Running 을 유지한다.
	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_LaserSpawn::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 상태를 떠나면(Active→Disabled 등) 살아있는 스폰체를 전부 제거한다. 스폰체는 서버 권위 액터라 권위 측에서만 파괴하고, 클라는 복제로 따라온다.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	for (const TObjectPtr<AActor>& Spawned : Instance.SpawnedActors)
	{
		if (IsValid(Spawned))
		{
			Spawned->Destroy();
		}
	}
	Instance.SpawnedActors.Reset();
}

#if WITH_EDITOR
FText FWxStateTreeTask_LaserSpawn::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Laser Spawn ({0})"),
		InstanceData->ActorClass ? FText::FromString(InstanceData->ActorClass->GetName()) : INVTEXT("(none)"));
}
#endif

// ── LaserAdvance ──────────────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_LaserAdvance::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 동적 스폰 액터라 복원할 포즈가 없다. 곧장 Running 으로 들어가 Tick 이 이동을 구동하게 한다.
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_LaserAdvance::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// 이동은 서버 권위 사건이다. 클라는 복제로 위치를 추종하므로 진행시키지 않는다.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Running;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const FVector Step = Instance.Velocity * DeltaTime;
	for (AActor* Actor : Instance.Actors)
	{
		if (IsValid(Actor))
		{
			Actor->AddActorWorldOffset(Step, false);
		}
	}

	// 완료 전이 없는 머무는 상태다. 항상 Running 을 유지한다.
	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_LaserAdvance::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return INVTEXT("Wx Laser Advance");
}
#endif
