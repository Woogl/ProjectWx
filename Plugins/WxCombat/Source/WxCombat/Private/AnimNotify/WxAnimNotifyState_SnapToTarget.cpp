// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_SnapToTarget.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "MotionWarpingComponent.h"
#include "Targeting/WxRootMotionModifier_SnapToTarget.h"

namespace
{
	const FName LocationWarpTargetName = TEXT("SnapToTarget_Location");
	const FName RotationWarpTargetName = TEXT("SnapToTarget_Rotation");
}

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

	const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify();
	if (!NotifyEvent)
	{
		return;
	}

	const float WarpStartTime = NotifyEvent->GetTriggerTime();
	const float WarpEndTime = NotifyEvent->GetEndTriggerTime();

	// 엔진은 한 워프 타겟 안에서 이동과 응시가 같은 위치를 공유하므로, LocationOffset을 쓰려면 modifier를 둘로 쪼개야 한다.
	// bSubtractRemainingRootMotion은 종료 시점 잔여 루트 모션이 타겟 너머로 미는 것을 막는다.

	if (bSnapLocation)
	{
		UWxRootMotionModifier_SnapToTarget* LocationModifier = NewObject<UWxRootMotionModifier_SnapToTarget>(MotionWarpingComp);
		LocationModifier->Animation = Animation;
		LocationModifier->StartTime = WarpStartTime;
		LocationModifier->EndTime = WarpEndTime;
		LocationModifier->WarpTargetName = LocationWarpTargetName;
		LocationModifier->TargetingPreset = TargetingPreset;
		LocationModifier->LocationOffset = LocationOffset;
		LocationModifier->bWarpTranslation = true;
		LocationModifier->bWarpRotation = false;
		LocationModifier->bSubtractRemainingRootMotion = true;

		MotionWarpingComp->AddModifier(LocationModifier);
	}

	// 오프셋 없이 대상 중심을 응시하므로 오너가 응시점에 겹치는 특이점이 없다.
	if (bSnapRotation)
	{
		UWxRootMotionModifier_SnapToTarget* RotationModifier = NewObject<UWxRootMotionModifier_SnapToTarget>(MotionWarpingComp);
		RotationModifier->Animation = Animation;
		RotationModifier->StartTime = WarpStartTime;
		RotationModifier->EndTime = WarpEndTime;
		RotationModifier->WarpTargetName = RotationWarpTargetName;
		RotationModifier->TargetingPreset = TargetingPreset;
		RotationModifier->LocationOffset = FVector::ZeroVector;
		RotationModifier->bWarpTranslation = false;
		RotationModifier->bWarpRotation = true;
		RotationModifier->RotationType = EMotionWarpRotationType::Facing;
		RotationModifier->bSubtractRemainingRootMotion = true;

		MotionWarpingComp->AddModifier(RotationModifier);
	}
}
