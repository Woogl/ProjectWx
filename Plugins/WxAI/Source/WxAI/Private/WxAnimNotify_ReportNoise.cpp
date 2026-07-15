// Copyright Woogle. All Rights Reserved.

#include "WxAnimNotify_ReportNoise.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Perception/AISense_Hearing.h"

void UWxAnimNotify_ReportNoise::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	// AI Perception 은 서버에서 동작하므로 서버에서만 보고한다.
	// fire-and-forget 스티뮬러스다. Loudness 1 고정, MaxRange 에 절대 거리(cm) 지정, Instigator=소유 액터(청취자-소음원 팀 소속 판정).
	if (Owner->HasAuthority())
	{
		UAISense_Hearing::ReportNoiseEvent(Owner, Owner->GetActorLocation(), 1.f, Owner, HearingDistance);
	}
}
