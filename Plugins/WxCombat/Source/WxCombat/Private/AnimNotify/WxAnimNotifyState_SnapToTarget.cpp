// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_SnapToTarget.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "Targeting/WxLockOnManagerComponent.h"
#include "MotionWarpingComponent.h"
#include "RootMotionModifier.h"
#include "RootMotionModifier_SkewWarp.h"
#include "TargetingSystem/TargetingSubsystem.h"

const FName UWxAnimNotifyState_SnapToTarget::DefaultWarpTargetName = TEXT("SnapToTarget");

void UWxAnimNotifyState_SnapToTarget::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (!bSnapLocation && !bSnapRotation)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	UMotionWarpingComponent* MotionWarpingComp = Owner->FindComponentByClass<UMotionWarpingComponent>();
	if (!MotionWarpingComp)
	{
		return;
	}

	// 락온 대상(우선). 부위(SceneComponent)에서 소유 액터를 환원해 쓴다.
	AActor* LockOnTarget = nullptr;
	if (UWxLockOnManagerComponent* LockOnComp = UWxLockOnManagerComponent::FindComponent(Owner))
	{
		if (const USceneComponent* LockOnTargetComponent = LockOnComp->GetLockOnTarget())
		{
			LockOnTarget = LockOnTargetComponent->GetOwner();
		}
	}

	// TargetingPreset 쿼리 결과로 "스냅 가능한 타겟팅 범위"를 판정한다.
	// 락온 대상이 결과에 포함되어 있으면 범위 안으로 보고 위치 스냅을 허용하고,
	// 결과 밖이면 회전만 적용한다.
	TArray<AActor*> TargetingResults;
	if (TargetingPreset)
	{
		if (UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(Owner->GetWorld()))
		{
			FTargetingSourceContext SourceContext;
			SourceContext.SourceActor = Owner;
			SourceContext.InstigatorActor = Owner;

			FTargetingRequestHandle Handle = TargetingSubsystem->MakeTargetRequestHandle(TargetingPreset, SourceContext);
			TargetingSubsystem->ExecuteTargetingRequestWithHandle(Handle);
			TargetingSubsystem->GetTargetingResultsActors(Handle, TargetingResults);
			TargetingSubsystem->ReleaseTargetRequestHandle(Handle);
		}
	}

	AActor* FacingTarget = LockOnTarget;
	if (!FacingTarget && TargetingResults.Num() > 0)
	{
		FacingTarget = TargetingResults[0];
	}

	if (!FacingTarget)
	{
		return;
	}

	const bool bFacingTargetIsLockOn = (FacingTarget == LockOnTarget);

	// TargetingPreset이 설정되어 있을 때만 범위 체크. Preset이 없으면 판정 근거가 없으므로 허용(기본 동작).
	const bool bTargetInSnapRange = !TargetingPreset || TargetingResults.Contains(FacingTarget);

	// 위치 워프 디싱크는 클라가 예측하는 플레이어 폰에서만 문제다. 그 경우에만 복제되는 락온 대상으로 제한하고,
	// 서버 권위로만 도는 AI 등은 폴백 위치 스냅을 유지한다. IsPlayerControlled 는 소유 클라/서버 양쪽에서 일관된다.
	const APawn* OwnerPawn = Cast<APawn>(Owner);
	const bool bRequireLockOnForTranslation = OwnerPawn && OwnerPawn->IsPlayerControlled();
	const bool bShouldWarpTranslation = bSnapLocation && bTargetInSnapRange && (!bRequireLockOnForTranslation || bFacingTargetIsLockOn);

	// 접근/회전 모두 수평(yaw) 전용. ground 전투의 안전한 표준이며, 작은 높이차는 캡슐 step-up/CMC 가 흡수한다.
	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector TargetLocation = FacingTarget->GetActorLocation();
	FVector Direction = TargetLocation - OwnerLocation;
	Direction.Z = 0.0;

	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify();
	if (!NotifyEvent)
	{
		return;
	}

	const float CurrentDistance = Direction.Size();
	const FVector DirectionNorm = Direction / CurrentDistance;
	const float StopDistance = FMath::Max(0.0f, CurrentDistance - MinDistance);
	const FVector WarpLocation = OwnerLocation + DirectionNorm * StopDistance;
	const FRotator WarpRotation = Direction.Rotation();

	MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(DefaultWarpTargetName, WarpLocation, WarpRotation);

	URootMotionModifier_SkewWarp::AddRootMotionModifierSkewWarp(
		MotionWarpingComp,
		Animation,
		NotifyEvent->GetTriggerTime(),
		NotifyEvent->GetEndTriggerTime(),
		DefaultWarpTargetName,
		EWarpPointAnimProvider::None,
		FTransform::Identity,
		NAME_None,
		bShouldWarpTranslation,
		true,
		bSnapRotation,
		// Facing 모드는 (WarpLoc - CharLoc).GetSafeNormal2D()로 매 틱 방향을 재계산하는데,
		// 캐릭터가 WarpLoc에 가까워지면 2D 벡터가 0에 수렴해 정규화가 실패하고
		// 회전이 엉뚱한 방향으로 튄다(RootMotionModifier.cpp:433).
		// 근거리 콤보에서 이 축퇴 구간이 프레임 단위로 노출되므로, 캡처된 회전값을
		// 그대로 쓰는 Default 모드를 사용한다.
		EMotionWarpRotationType::Default,
		EMotionWarpRotationMethod::Slerp,
		1.0f,
		0.0f
	);
}

void UWxAnimNotifyState_SnapToTarget::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	// Notify 종료 이후의 잔여 forward 루트 모션이 캐릭터를 타겟 너머로 밀지 않도록, "지금 있는 자리"를 hold 지점으로 둔다.
	// NotifyBegin 시점이 아니라 종료 시점의 실제 위치를 쓰므로, 메인 워프가 도달했든 콜리전으로 막혔든 2차 돌진이 없다.
	// 위치 스냅을 쓰는 경우에만 적용한다(bSnapLocation 은 인스턴스 공유에 안전한 CDO 프로퍼티).
	if (!bSnapLocation || !MeshComp || !Animation)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	// 위치 스냅이 실제로 걸렸을 조건에 맞춰 hold 도 게이팅한다(범위 체크 제외).
	// 플레이어 폰은 락온 대상이 있어야 위치 스냅이 적용되므로(NotifyBegin 과 동일 규칙), 락온이 없으면 hold(전진 억제)도 걸지 않는다.
	const APawn* OwnerPawn = Cast<APawn>(Owner);
	if (OwnerPawn && OwnerPawn->IsPlayerControlled())
	{
		const UWxLockOnManagerComponent* LockOnComp = UWxLockOnManagerComponent::FindComponent(Owner);
		if (!LockOnComp || !LockOnComp->GetLockOnTarget())
		{
			return;
		}
	}

	UMotionWarpingComponent* MotionWarpingComp = Owner->FindComponentByClass<UMotionWarpingComponent>();
	if (!MotionWarpingComp)
	{
		return;
	}

	const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify();
	if (!NotifyEvent)
	{
		return;
	}

	const float TailStart = NotifyEvent->GetEndTriggerTime();
	const float TailEnd = Animation->GetPlayLength();
	if (TailEnd <= TailStart)
	{
		return;
	}

	const FVector HoldLocation = Owner->GetActorLocation();
	MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(DefaultWarpTargetName, HoldLocation, Owner->GetActorRotation());

	URootMotionModifier_SkewWarp::AddRootMotionModifierSkewWarp(
		MotionWarpingComp,
		Animation,
		TailStart,
		TailEnd,
		DefaultWarpTargetName,
		EWarpPointAnimProvider::None,
		FTransform::Identity,
		NAME_None,
		true,
		// 수평(XY)만 hold 한다. 수직(중력/착지)은 자연스럽게 흐르도록 Z 는 무시.
		true,
		false,
		EMotionWarpRotationType::Default,
		EMotionWarpRotationMethod::Slerp,
		1.0f,
		0.0f
	);
}
