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

	// 이동과 회전을 별도 modifier 로 분리한다. 엔진은 한 워프 타겟 안에서 이동·응시가 같은 위치를 공유해 분리할 수 없으므로,
	// LocationOffset 은 이동 modifier 에만 적용하고 회전 modifier 는 오프셋 없이 대상 중심을 Facing 으로 응시한다.
	// 두 채널이 직교해 ProcessRootMotion 에서 순차 합성된다. 게임 로직(락온·타겟팅 판정, 워프 타겟 등록)은 각 modifier 가 소유한다.
	// 종료 시점 잔여 루트 모션이 타겟 너머로 밀지 않도록 bSubtractRemainingRootMotion 으로 애니 종료 시점 도달로 보정한다.

	// 이동: 대상 + LocationOffset 지점으로 접근한다. 범위·락온·MP 게이팅은 modifier 가 판정한다.
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

	// 회전: 오프셋 없이 대상 중심을 응시한다(거리·LocationOffset 무관). 오너가 응시점에 겹치지 않아 특이점이 없다.
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
