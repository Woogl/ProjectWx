// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxRootMotionModifier_SnapToTarget.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "MotionWarpingComponent.h"
#include "Targeting/WxLockOnManagerComponent.h"
#include "TargetingSystem/TargetingSubsystem.h"

void UWxRootMotionModifier_SnapToTarget::OnStateChanged(ERootMotionModifierState LastState)
{
	Super::OnStateChanged(LastState);

	// Active 로 막 진입한 프레임에만 스냅 타겟을 판정해 워프 타겟을 등록한다.
	// (부모 Warp::Update 가 같은 WarpTargetName 으로 픽업해 루트 모션을 보정한다.)
	if (LastState == ERootMotionModifierState::Active || GetState() != ERootMotionModifierState::Active)
	{
		return;
	}

	AActor* Owner = GetActorOwner();
	UMotionWarpingComponent* MotionWarpingComp = GetOwnerComponent();
	if (!Owner || !MotionWarpingComp)
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
	// 락온 대상이 결과에 포함되어 있으면 범위 안으로 보고 위치 스냅을 허용하고, 결과 밖이면 회전만 적용한다.
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

	// 위치 워프 디싱크는 클라가 예측하는 플레이어 폰에서만 문제다.
	// 그 경우에만 복제되는 락온 대상으로 제한하고, 서버 권위로만 도는 AI 등은 폴백 위치 스냅을 유지한다.
	// IsPlayerControlled 는 소유 클라/서버 양쪽에서 일관된다.
	const APawn* OwnerPawn = Cast<APawn>(Owner);
	const bool bRequireLockOnForTranslation = OwnerPawn && OwnerPawn->IsPlayerControlled();
	const bool bShouldWarpTranslation = bWarpTranslation && bTargetInSnapRange && (!bRequireLockOnForTranslation || bFacingTargetIsLockOn);

	// 접근/회전 모두 수평(yaw) 전용. ground 전투의 안전한 표준이며, 작은 높이차는 캡슐 step-up/CMC 가 흡수한다.
	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector TargetLocation = FacingTarget->GetActorLocation();
	FVector Direction = TargetLocation - OwnerLocation;
	Direction.Z = 0.0;

	if (Direction.IsNearlyZero())
	{
		return;
	}

	const float CurrentDistance = Direction.Size();
	const FVector DirectionNorm = Direction / CurrentDistance;
	const float StopDistance = FMath::Max(0.0f, CurrentDistance - MinDistance);
	const FVector WarpLocation = OwnerLocation + DirectionNorm * StopDistance;
	const FRotator WarpRotation = Direction.Rotation();

	// 게이팅 결과를 부모 워프 설정에 반영한다(범위 밖·플레이어 폰 락온 없음이면 회전만).
	bWarpTranslation = bShouldWarpTranslation;

	MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpLocation, WarpRotation);
}
