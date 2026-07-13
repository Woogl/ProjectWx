// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_Footstep.h"

#include "Components/SkeletalMeshComponent.h"
#include "ContextEffects/WxContextEffectsComponent.h"
#include "GameFramework/Actor.h"
#include "Perception/AISense_Hearing.h"
#include "WxGameplayTags.h"

void UWxAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	const FVector FootLocation = Owner->GetActorLocation();

	// AI 소음 신호(게임플레이): AI Perception 은 서버에서 동작하므로 서버에서만 보고한다.
	// fire-and-forget 스티뮬러스라(SendGameplayEvent 와 동급) 노티파이가 직접 쏜다 — 코스메틱 시스템과 분리.
	// Loudness 1 고정, MaxRange 에 절대 거리(cm) 지정, Instigator=소유 액터(청취자-소음원 팀 소속 판정).
	if (Owner->HasAuthority())
	{
		UAISense_Hearing::ReportNoiseEvent(Owner, FootLocation, 1.f, Owner, HearingDistance);
	}

	// 코스메틱(표면별 발소리/VFX)은 범용 컨텍스트 이펙트 시스템에 위임한다.
	if (const UWxContextEffectsComponent* ContextEffects = Owner->FindComponentByClass<UWxContextEffectsComponent>())
	{
		ContextEffects->TriggerEffect(WxGameplayTags::AnimNotify_Footstep, FootLocation);
	}
}
