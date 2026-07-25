// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmickStateTreeNodes.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Gimmick/WxGimmick.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "NiagaraFunctionLibrary.h"
#include "Spawnable/WxSpawner.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"
#include "WxGameplayTags.h"
#include "WxWorldModule.h"

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

	// 이 진입을 스냅·스킵으로 처리해야 하는가 — StateTree 시작/복원/레이트조인(SourceStateID 무효)이거나, 세이브 복원(호스트 AWxGimmick 가 상태 태그와 함께 보내는 StateTree.Restore 마커)이면 참.
	// Required Event 전이로 저장 상태에 진입할 때 라이브 발동처럼 보여도 일회성 효과를 발동하지 않고 스냅하도록.
	bool IsInitialOrRestoreEntry(const FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
	{
		return !Transition.SourceStateID.IsValid() || Context.HasEventToProcess(WxGameplayTags::StateTree_Restore);
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

	UPrimitiveComponent* TargetMesh = Instance.TargetMesh;
	if (!TargetMesh)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 활성 여부와 프롬프트를 둘 다 오너 기믹에 세팅한다 — 콜리전은 관여하지 않으므로 대상 메시의 콜리전 설정은 그대로 보존된다.
	// 꺼진 영역은 기믹의 활성 목록에서 빠져 스캐너의 다음 스캔에서 자연 탈락하고, 외곽선도 그때 스캐너가 끈다.
	// 프롬프트는 영역별로 담기므로 한 상태가 여러 영역을 켜도 서로 덮어쓰지 않는다. 끄는 상태는 빈 텍스트를 넘겨 이전 상태가 남긴 문구를 지운다(다시 켤 때 stale 값을 물려받지 않게).
	// 오너가 기믹이 아니면(비기믹 ST) 세팅할 대상이 없으므로 노옵이다.
	if (AWxGimmick* Gimmick = Cast<AWxGimmick>(Context.GetOwner()))
	{
		Gimmick->SetInteractionEnabled(TargetMesh, Instance.bEnable);
		Gimmick->SetCurrentInteractionPrompt(TargetMesh, Instance.bEnable ? Instance.Prompt : FText::GetEmpty());
	}

	// 토글은 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_EnableInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 대상 메시는 보통 바인딩이라 런타임 포인터가 비어 있다. 바인딩 소스명을 우선 보이고, 직접 지정 시 그 이름으로 폴백.
	FText TargetText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, TargetMesh)), Formatting);
	if (TargetText.IsEmpty())
	{
		TargetText = InstanceData->TargetMesh ? FText::FromString(InstanceData->TargetMesh->GetName()) : INVTEXT("none");
	}

	// 상호작용을 켜고 프롬프트가 있으면 함께 보여, 상태별 프롬프트를 노드 설명에서 바로 확인할 수 있게 한다.
	if (InstanceData->bEnable && !InstanceData->Prompt.IsEmpty())
	{
		return FText::Format(INVTEXT("Enable \"{1}\" Interaction ({0})"), TargetText, InstanceData->Prompt);
	}

	return FText::Format(INVTEXT("{0} ({1})"), InstanceData->bEnable ? INVTEXT("Enable Interaction") : INVTEXT("Disable Interaction"), TargetText);
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

	return InstanceData->bEnable ? INVTEXT("Enable Player Input") : INVTEXT("Disable Player Input");
}
#endif

// ── ComponentMove ───────────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_ComponentMove::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USceneComponent* Component = Instance.TargetComponent;
	if (!Component)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 아키타입 조회가 상수 시간이 아니라 여기서 1회만 구해 인스턴스 데이터에 캐시하고, Tick 은 이 값을 읽기만 한다.
	Instance.TargetLocation = GetMoveAnchor(Component) + Instance.LocalOffset;

	// 초기 진입(StateTree 시작/복원)·길이 0·이미 목표면 애니 없이 즉시 스냅하고 곧바로 완료한다.
	const bool bInitialEntry = IsInitialOrRestoreEntry(Context, Transition);
	const bool bReachNow = bInitialEntry || Instance.Duration <= 0.f || Component->GetRelativeLocation().Equals(Instance.TargetLocation);
	if (bReachNow)
	{
		Component->SetRelativeLocation(Instance.TargetLocation);
		return EStateTreeRunStatus::Succeeded;
	}

	// 라이브 전이: 속도를 시작(현재)→목표 실제 거리/Duration 으로 1회 산출한다(LocalOffset 크기가 아니라 실제 거리라, 목표가 아키타입인 닫기도 0 이 아니다).
	Instance.MoveSpeed = (Instance.TargetLocation - Component->GetRelativeLocation()).Size() / Instance.Duration;

	// 라이브 전이면 Tick 이 고정 속도로 슬라이드하다 도달 시 완료한다.
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

	// 목표는 EnterState 에서 캐시한 값을 쓴다(아키타입 조회를 매 틱 반복하지 않는다).
	const FVector& Target = Instance.TargetLocation;
	FVector NewLocation = Component->GetRelativeLocation();

	// 도달 전까지 EnterState 에서 산출한 고정 속도로 슬라이드한다.
	if (!NewLocation.Equals(Target))
	{
		NewLocation = FMath::VInterpConstantTo(NewLocation, Target, DeltaTime, Instance.MoveSpeed);
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
		ComponentText = InstanceData->TargetComponent ? FText::FromString(InstanceData->TargetComponent->GetName()) : INVTEXT("none");
	}

	return FText::Format(INVTEXT("Component Move ({0})"), ComponentText);
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

	// 초기 진입(StateTree 시작/복원/레이트조인)이면 목표 포인트로 즉시 스냅한다(복제/복원된 State 의 끝점을 정확히 복원).
	const bool bInitialEntry = IsInitialOrRestoreEntry(Context, Transition);
	if (bInitialEntry)
	{
		Instance.CurrentDistance = TargetDistance;
		Instance.MoveSpeed = 0.f;
		Component->SetWorldLocation(Spline->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World));
		// 목표 포인트로 스냅했으니 곧바로 완료한다(복원 시 단계 캐스케이드).
		return EStateTreeRunStatus::Succeeded;
	}

	// 라이브 전이: 플랫폼의 실제 현재 위치에 해당하는 스플라인 거리를 시작점으로 잡는다(vertex 로 양자화하지 않아 이동 중 반전도 스냅 없이 현재 지점에서 출발).
	const float StartDistance = Spline->GetDistanceAlongSplineAtLocation(Component->GetComponentLocation(), ESplineCoordinateSpace::World);

	// 속도는 시작→목표 남은 거리/Duration 으로 EnterState 에서 1회 산출한다(Duration 0 이하면 아래에서 즉시 스냅).
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
		SplineText = InstanceData->Spline ? FText::FromString(InstanceData->Spline->GetName()) : INVTEXT("none");
	}

	return FText::Format(INVTEXT("Component Spline Move ({0} → point {1})"), SplineText, FText::AsNumber(InstanceData->TargetPointIndex));
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

	// 초기 진입(StateTree 시작/복원/레이트조인)이면 끝 프레임으로 스냅해 발동 완료 포즈를 복원하고 곧바로 완료한다.
	const bool bInitialEntry = IsInitialOrRestoreEntry(Context, Transition);
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

	return FText::Format(INVTEXT("Play Animation ({0})"),
		InstanceData->Animation ? FText::FromString(InstanceData->Animation->GetName()) : INVTEXT("none"));
}
#endif

// ── MoveInteractorToTarget ─────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_MoveInteractorToTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 차단 기록을 비운 상태로 시작한다. 아래 어느 조기 완료 경로로 빠지든 ExitState 가 걸지도 않은 차단을 해제하는 일이 없어야 한다.
	Instance.BlockedController = nullptr;
	Instance.BlockedAbilitySystem = nullptr;

	// 초기 진입(StateTree 시작/복원/레이트조인)이면 이동 없이 곧바로 완료한다(발동 순간에만 동작; InteractingCharacter 는 비영속이라 복원 시 비어 있음).
	if (IsInitialOrRestoreEntry(Context, Transition))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 당사자는 오너 기믹이 권위 측에서 기록해 복제한 값이라 모든 피어가 같은 대상을 본다(에셋 배선 없음).
	const AWxGimmick* Gimmick = Cast<AWxGimmick>(Context.GetOwner());
	ACharacter* Character = Gimmick ? Gimmick->GetInteractingCharacter() : nullptr;

	// 당사자가 없으면(비캐릭터 상호작용, 비기믹 오너 등) 상태가 갇히지 않게 곧바로 완료한다.
	if (!Character)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Move Interactor To Target: 오너 기믹의 InteractingCharacter 가 비어 있다 — 완료로 넘어간다."));
		return EStateTreeRunStatus::Succeeded;
	}

	// 스크립트 이동 동안 CMC 잔여 속도를 제거한다.
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	// 이동/응시 동안 로컬 플레이어의 입력을 막는다(카메라 look 은 별개 게이트라 유지). ExitState 에서 짝 해제한다.
	//  - 이동: AController::SetIgnoreMoveInput 로 AddMovementInput 을 무시.
	//  - 어빌리티+점프: ASC 의 BlockAbilitiesWithTags(Ability) — 액션 어빌리티가 연출 중 서로를 막는 것과 동일한 GAS 순정 관례이며, 캐릭터의 CanJumpInternal 이 이미 AreAbilityTagsBlocked(Ability) 로 점프를 막으므로 점프도 함께 차단된다.
	// 입력이 실제로 생기고 예측이 발동을 게이트하는 로컬 컨트롤 인스턴스에서만 건다(소유 클라가 막으면 서버로 활성화가 전송되지 않아 서버 차단이 불필요). 스냅·이동 두 경로 모두에서 걸어 ExitState 해제와 짝을 맞춘다.
	// 차단에 성공한 대상은 그때그때 인스턴스에 기록해 둔다 — ExitState 는 이 기록만 보고 해제하므로, 그 사이 캐릭터가 소멸·언포제스돼도 카운터가 새지 않는다.
	if (Character->IsLocallyControlled())
	{
		if (AController* Controller = Character->GetController())
		{
			Controller->SetIgnoreMoveInput(true);
			Instance.BlockedController = Controller;
		}
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character))
		{
			ASC->BlockAbilitiesWithTags(FGameplayTagContainer(WxGameplayTags::Ability));
			Instance.BlockedAbilitySystem = ASC;
		}
	}

	// 목표 = 앵커(또는 오너) 트랜스폼 ∘ 상대오프셋. 모든 머신에서 동일하게 합성돼 수렴한다.
	const FTransform Anchor = Instance.AnchorComponent ? Instance.AnchorComponent->GetComponentTransform() : Gimmick->GetActorTransform();
	const FVector TargetLocation = Anchor.TransformPosition(Instance.RelativeLocation);
	const float TargetYaw = (Anchor.GetRotation() * Instance.RelativeRotation.Quaternion()).Rotator().Yaw;

	const FVector StartLocation = Character->GetActorLocation();

	// Duration 0 이하·이미 목표면 즉시 스냅해 곧바로 완료, 아니면 시작→목표 실제 거리/시간으로 등속을 1회 산출하고 Tick 이 슬라이드한다.
	const bool bReachNow = Instance.Duration <= 0.f || StartLocation.Equals(TargetLocation);
	if (bReachNow)
	{
		Character->SetActorLocation(TargetLocation);
		if (Instance.bAlignRotation)
		{
			FRotator Rotation = Character->GetActorRotation();
			Rotation.Yaw = TargetYaw;
			Character->SetActorRotation(Rotation);
		}
		return EStateTreeRunStatus::Succeeded;
	}

	Instance.MoveSpeed = (TargetLocation - StartLocation).Size() / Instance.Duration;
	if (Instance.bAlignRotation)
	{
		const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(Character->GetActorRotation().Yaw, TargetYaw));
		Instance.TurnSpeed = YawDelta / Instance.Duration;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_MoveInteractorToTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AWxGimmick* Gimmick = Cast<AWxGimmick>(Context.GetOwner());
	ACharacter* Character = Gimmick ? Gimmick->GetInteractingCharacter() : nullptr;
	if (!Character)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FTransform Anchor = Instance.AnchorComponent ? Instance.AnchorComponent->GetComponentTransform() : Gimmick->GetActorTransform();
	const FVector TargetLocation = Anchor.TransformPosition(Instance.RelativeLocation);
	const float TargetYaw = (Anchor.GetRotation() * Instance.RelativeRotation.Quaternion()).Rotator().Yaw;

	// 도달 전까지 EnterState 에서 산출한 등속으로 슬라이드한다.
	FVector NewLocation = Character->GetActorLocation();
	if (!NewLocation.Equals(TargetLocation))
	{
		NewLocation = FMath::VInterpConstantTo(NewLocation, TargetLocation, DeltaTime, Instance.MoveSpeed);
		Character->SetActorLocation(NewLocation);
	}

	if (Instance.bAlignRotation)
	{
		FRotator Rotation = Character->GetActorRotation();
		Rotation.Yaw = FMath::FixedTurn(Rotation.Yaw, TargetYaw, Instance.TurnSpeed * DeltaTime);
		Character->SetActorRotation(Rotation);
	}

	// 아직 도달 전이면 계속 이동한다.
	if (!NewLocation.Equals(TargetLocation))
	{
		return EStateTreeRunStatus::Running;
	}

	// 도달: yaw 를 목표로 스냅해 마무리하고 상태를 완료시킨다.
	if (Instance.bAlignRotation)
	{
		FRotator Rotation = Character->GetActorRotation();
		Rotation.Yaw = TargetYaw;
		Character->SetActorRotation(Rotation);
	}
	return EStateTreeRunStatus::Succeeded;
}

void FWxStateTreeTask_MoveInteractorToTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// EnterState 에서 건 입력 차단을 해제한다. SetIgnoreMoveInput·BlockAbilitiesWithTags 모두 스택 카운터라 진입 시의 +1 과 짝을 맞춰야 한다.
	// 해제 대상을 오너 기믹의 InteractingCharacter 로 되짚지 않는 이유: 그 값은 권위 측이 언제든 갱신하는 라이브 멤버라 진입 시점의 스냅샷이 아니다.
	// 이동 중 캐릭터가 파괴되거나(Tick 이 Failed 반환) 언포제스되면 그 경로로는 대상을 잃어 해제가 통째로 스킵되고, 컨트롤러에 쌓인 카운터가 리스폰 후에도 남는다.
	// 그래서 진입 때 차단에 성공한 대상 자체를 기록해 두고, 여기서는 그 기록만 근거로 해제한다(기록이 비어 있으면 애초에 걸지 않은 것이다).
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (AController* Controller = Instance.BlockedController.Get())
	{
		Controller->SetIgnoreMoveInput(false);
	}
	if (UAbilitySystemComponent* ASC = Instance.BlockedAbilitySystem.Get())
	{
		ASC->UnBlockAbilitiesWithTags(FGameplayTagContainer(WxGameplayTags::Ability));
	}

	Instance.BlockedController = nullptr;
	Instance.BlockedAbilitySystem = nullptr;
}

#if WITH_EDITOR
FText FWxStateTreeTask_MoveInteractorToTarget::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 이동 대상은 오너 기믹에서 읽으므로 표시할 것이 없다. 상태마다 갈리는 건 목표 앵커라 그것을 보인다.
	// 앵커는 보통 바인딩이라 런타임 포인터가 비어 있다. 바인딩 소스명을 우선 보이고, 비우면 오너 트랜스폼 기준이므로 owner 로 폴백.
	FText AnchorText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, AnchorComponent)), Formatting);
	if (AnchorText.IsEmpty())
	{
		AnchorText = InstanceData->AnchorComponent ? FText::FromString(InstanceData->AnchorComponent->GetName()) : INVTEXT("owner");
	}

	return FText::Format(INVTEXT("Move Interactor To Target ({0})"), AnchorText);
}
#endif

// ── PlayInteractorMontage ──────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_PlayInteractorMontage::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 초기 진입(StateTree 시작/복원/레이트조인)이면 재생 없이 곧바로 완료한다(발동 순간에만 재생; InteractingCharacter 는 비영속이라 복원 시 비어 있음).
	if (IsInitialOrRestoreEntry(Context, Transition))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 당사자는 오너 기믹이 권위 측에서 기록해 복제한 값이라 모든 피어가 같은 대상을 본다(에셋 배선 없음).
	const AWxGimmick* Gimmick = Cast<AWxGimmick>(Context.GetOwner());
	ACharacter* Character = Gimmick ? Gimmick->GetInteractingCharacter() : nullptr;
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;

	// 대상/몽타주/애님인스턴스가 없으면 상태가 갇히지 않게 곧바로 완료한다.
	if (!AnimInstance || !Instance.Montage)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 각 머신이 메시 AnimInstance 로 로컬 재생한다(복제형 PlayAnimMontage 아님 — 모든 피어가 각자 재생해 중복이 없다).
	AnimInstance->Montage_Play(Instance.Montage);

	// Tick 이 종료를 폴링해 완료시킨다.
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayInteractorMontage::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AWxGimmick* Gimmick = Cast<AWxGimmick>(Context.GetOwner());
	ACharacter* Character = Gimmick ? Gimmick->GetInteractingCharacter() : nullptr;
	if (!Character)
	{
		return EStateTreeRunStatus::Failed;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance || !Instance.Montage)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 재생이 끝나면(또는 다른 몽타주로 교체되면) 상태를 완료시킨다.
	return AnimInstance->Montage_IsPlaying(Instance.Montage) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayInteractorMontage::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Play Interactor Montage ({0})"),
		InstanceData->Montage ? FText::FromString(InstanceData->Montage->GetName()) : INVTEXT("none"));
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

	// 재생 종료를 소유 기믹에 통지한다(권위 측만). 종료를 아는 주체가 재생 소유자(이 태스크)뿐이므로 직접 발행한다.
	// HandleLevelSequenceFinished 를 구현하지 않는 기믹은 기본 노옵이라 무해하다(예: OnComplete 전이로 진행하는 기믹).
	void NotifyHostSequenceFinished(FStateTreeExecutionContext& Context)
	{
		AWxGimmick* OwnerGimmick = Cast<AWxGimmick>(Context.GetOwner());
		if (OwnerGimmick && OwnerGimmick->HasAuthority())
		{
			OwnerGimmick->HandleLevelSequenceFinished();
		}
	}
}

EStateTreeRunStatus FWxStateTreeTask_PlayLevelSequence::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 초기 진입(StateTree 시작/복원/레이트조인)이면 재생하지 않고 침묵 완료한다 — 복원 시 호스트에 통지하지 않는다(라이브 복귀 오발화 방지).
	const bool bInitialEntry = IsInitialOrRestoreEntry(Context, Transition);
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 라이브 진입이지만 재생할 게 없으면, 호스트가 Playing 에 갇히지 않게 곧장 종료를 통지하고 완료한다.
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!Instance.LevelSequence || !World)
	{
		NotifyHostSequenceFinished(Context);
		return EStateTreeRunStatus::Succeeded;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	ALevelSequenceActor* NewSequenceActor = nullptr;
	Instance.Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Instance.LevelSequence, PlaybackSettings, NewSequenceActor);
	Instance.SequenceActor = NewSequenceActor;

	// 플레이어 생성 실패면 정리하고, 라이브 진입이므로 호스트에 종료를 통지한 뒤 완료한다.
	if (!Instance.Player)
	{
		FinishSequencePlayback(Instance);
		NotifyHostSequenceFinished(Context);
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

	// 재생이 끝났다. 시퀀스를 정리하고 권위 측이면 소유 기믹에 종료를 통지해 State 복귀(예: Idle)를 구동하게 한 뒤 완료한다.
	FinishSequencePlayback(Instance);
	NotifyHostSequenceFinished(Context);
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

	return FText::Format(INVTEXT("Play Level Sequence ({0})"),
		InstanceData->LevelSequence ? FText::FromString(InstanceData->LevelSequence->GetName()) : INVTEXT("none"));
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
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 초기 진입(StateTree 시작/복원/레이트조인)이면 기본적으로 재생하지 않고 곧바로 완료한다(발동 순간에만 울림). bPlayOnRestore 면 복원/시작 진입에서도 재생한다(지속 사운드용).
	const bool bInitialEntry = IsInitialOrRestoreEntry(Context, Transition);
	if (bInitialEntry && !Instance.bPlayOnRestore)
	{
		return EStateTreeRunStatus::Succeeded;
	}

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

	return FText::Format(INVTEXT("Play Sound ({0})"),
		InstanceData->Sound ? FText::FromString(InstanceData->Sound->GetName()) : INVTEXT("none"));
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

	return FText::Format(INVTEXT("Spawn Niagara ({0})"),
		InstanceData->Niagara ? FText::FromString(InstanceData->Niagara->GetName()) : INVTEXT("none"));
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
	// 초기 진입(StateTree 시작/복원/레이트조인)이면 스폰을 재실행하지 않고 곧바로 완료한다(발동 순간에만 스폰).
	const bool bInitialEntry = IsInitialOrRestoreEntry(Context, Transition);
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
			UE_LOG(LogWxWorld, Warning, TEXT("Trigger Spawners: TargetSpawner is null or not loaded."));
		}
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_TriggerSpawners::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Trigger Spawners ({0})"), FText::AsNumber(InstanceData->Spawners.Num()));
}
#endif

// ── SpawnActor ────────────────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_SpawnActor::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 스폰체는 Transient 라 복원할 포즈가 없다. 누적기를 Interval 로 채워 진입 첫 틱에 즉시 1회 스폰한다(이후 반복 여부는 Tick 의 주기 처리가 정한다).
	Instance.TimeSinceLastSpawn = Instance.Interval;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_SpawnActor::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
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
	// Interval 양수면 한 주기 차감해 반복 스폰하고, 0(일회성)이면 누적기를 음의 극값으로 묶어 다시 임계에 도달하지 못하게 해 1회만 스폰한다.
	Instance.TimeSinceLastSpawn = Instance.Interval > 0.f ? Instance.TimeSinceLastSpawn - Instance.Interval : -TNumericLimits<float>::Max();

	UWorld* World = Owner->GetWorld();
	if (!World || !Instance.ActorClass)
	{
		return EStateTreeRunStatus::Running;
	}

	// 스폰 위치·회전·크기는 LocalSpawnTransform 을 오너 월드 트랜스폼에 합성해 정한다(Identity 면 오너 트랜스폼 그대로).
	const FTransform SpawnTransform = Instance.LocalSpawnTransform * Owner->GetActorTransform();
	AActor* Spawned = World->SpawnActorDeferred<AActor>(Instance.ActorClass, SpawnTransform, Owner, Owner->GetInstigator(), Instance.SpawnCollisionHandlingOverride);
	if (!Spawned)
	{
		return EStateTreeRunStatus::Running;
	}

	// Lifetime 양수면 그만큼 살다 자동 파괴된다(0 이하면 직접 파괴/이탈 정리에 맡김).
	if (Instance.Lifetime > 0.f)
	{
		Spawned->SetLifeSpan(Instance.Lifetime);
	}
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

void FWxStateTreeTask_SpawnActor::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 스폰·파괴는 서버 권위 사건이라 권위 측만 처리한다(클라는 SpawnedActors 가 비어 있고 복제로 추종).
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// bDestroyOnExit 면 상태를 떠날 때(Active→Disabled 등) 추적 중인 스폰체를 전부 제거한다. 끄면 남겨 각자 Lifetime 으로 끝까지 살다 자동 파괴되게 둔다.
	if (Instance.bDestroyOnExit)
	{
		for (const TObjectPtr<AActor>& Spawned : Instance.SpawnedActors)
		{
			if (IsValid(Spawned))
			{
				Spawned->Destroy();
			}
		}
	}
	Instance.SpawnedActors.Reset();
}

#if WITH_EDITOR
FText FWxStateTreeTask_SpawnActor::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Spawn Actor ({0})"),
		InstanceData->ActorClass ? FText::FromString(InstanceData->ActorClass->GetName()) : INVTEXT("none"));
}
#endif
