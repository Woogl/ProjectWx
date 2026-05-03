// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_SlowMotion.h"

#include "Time/WxTimeDilationComponent.h"

void UWxAnimNotifyState_SlowMotion::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UWxTimeDilationComponent::SetGlobalTimeDilationAuthoritative(MeshComp, TargetDilation);
}

void UWxAnimNotifyState_SlowMotion::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	UWxTimeDilationComponent::SetGlobalTimeDilationAuthoritative(MeshComp, 1.f);
}
