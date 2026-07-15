// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_SnapToTarget.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "MotionWarpingComponent.h"
#include "Targeting/WxRootMotionModifier_SnapToTarget.h"

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

	const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify();
	if (!NotifyEvent)
	{
		return;
	}

	// 게임 로직(락온·타겟팅 판정, 워프 타겟 등록)은 modifier 가 소유한다. 노티파이는 modifier 를 구성해 등록만 한다.
	// 종료 시점의 잔여 forward 루트 모션이 캐릭터를 타겟 너머로 밀지 않도록 bSubtractRemainingRootMotion 으로 애니 종료 시점 도달로 보정한다.
	UWxRootMotionModifier_SnapToTarget* Modifier = NewObject<UWxRootMotionModifier_SnapToTarget>(MotionWarpingComp);
	Modifier->Animation = Animation;
	Modifier->StartTime = NotifyEvent->GetTriggerTime();
	Modifier->EndTime = NotifyEvent->GetEndTriggerTime();
	Modifier->WarpTargetName = DefaultWarpTargetName;
	Modifier->TargetingPreset = TargetingPreset;
	Modifier->MinDistance = MinDistance;
	Modifier->bWarpTranslation = bSnapLocation;
	Modifier->bWarpRotation = bSnapRotation;
	Modifier->RotationType = EMotionWarpRotationType::Default;
	Modifier->bSubtractRemainingRootMotion = true;

	MotionWarpingComp->AddModifier(Modifier);
}
