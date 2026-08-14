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
	// Loudness 1 고정이라 MaxRange 가 곧 절대 거리(cm)이고, Instigator=소유 액터로 청취자-소음원 팀 소속을 판정한다.
	if (Owner->HasAuthority())
	{
		UAISense_Hearing::ReportNoiseEvent(Owner, Owner->GetActorLocation(), 1.f, Owner, HearingDistance);
	}
}
