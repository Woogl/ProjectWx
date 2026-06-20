// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmickStateTreeNodes.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Gimmick/WxGimmick.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"

namespace
{
	// 기준 포즈 = 컴포넌트 아키타입(BP/CDO 오서링)의 상대 위치. 런타임 위치가 어디든 안정적 앵커.
	FVector GetMoveAnchor(const USceneComponent* Component)
	{
		const USceneComponent* Archetype = Cast<USceneComponent>(Component->GetArchetype());
		return Archetype ? Archetype->GetRelativeLocation() : Component->GetRelativeLocation();
	}
}

FWxStateTreeTask_GimmickInteraction::FWxStateTreeTask_GimmickInteraction()
{
	// 인터랙션을 진입 시 1회 토글만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_GimmickInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (AWxGimmick* Gimmick = Cast<AWxGimmick>(Context.GetOwner()))
	{
		Gimmick->SetInteractionEnabled(Instance.bEnableInteraction);
	}

	// 토글은 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_GimmickInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Gimmick Interaction ({0})"),
		InstanceData->bEnableInteraction ? INVTEXT("enabled") : INVTEXT("disabled"));
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

// ── PlayFx ────────────────────────────────────────────────────────────────────

FWxStateTreeTask_PlayFx::FWxStateTreeTask_PlayFx()
{
	// 진입 시 1회 재생만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_PlayFx::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
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

	if (Instance.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(Owner, Instance.Sound, Owner->GetActorLocation());
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayFx::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Play Fx ({0})"),
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
