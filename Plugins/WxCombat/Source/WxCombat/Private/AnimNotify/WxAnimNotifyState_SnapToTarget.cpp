// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_SnapToTarget.h"
#include "Targeting/WxLockOnComponent.h"
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

	AActor* LockOnTarget = nullptr;
	if (UWxLockOnComponent* LockOnComp = UWxLockOnComponent::FindComponent(Owner))
	{
		LockOnTarget = LockOnComp->GetLockOnTarget();
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

	// TargetingPreset이 설정되어 있을 때만 범위 체크. Preset이 없으면 판정 근거가
	// 없으므로 스냅을 허용한다(기본 동작).
	const bool bTargetInSnapRange = !TargetingPreset || TargetingResults.Contains(FacingTarget);
	const bool bShouldWarpTranslation = bSnapLocation && bTargetInSnapRange;

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

	// Notify 윈도우 종료 이후의 잔여 forward 루트 모션이 캐릭터를 타겟 너머로 밀지 않도록,
	// 종료 시점부터 애니메이션 끝까지 같은 WarpLocation을 hold 지점으로 둔다. 메인 워프가
	// 도달시킨 위치에 다시 SkewWarp를 걸므로 잔여 트랜슬레이션이 0으로 스케일링된다.
	// 회전은 그대로 흐르도록 둔다.
	if (bShouldWarpTranslation)
	{
		const float TailStart = NotifyEvent->GetEndTriggerTime();
		const float TailEnd = Animation->GetPlayLength();
		if (TailEnd > TailStart)
		{
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
				true,
				false,
				EMotionWarpRotationType::Default,
				EMotionWarpRotationMethod::Slerp,
				1.0f,
				0.0f
			);
		}
	}
}
