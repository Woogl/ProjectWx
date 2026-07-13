// Copyright Woogle. All Rights Reserved.

#include "ContextEffects/WxAnimNotify_ContextEffects.h"

#include "ContextEffects/WxContextEffectsComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UWxAnimNotify_ContextEffects::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	// 실제 처리는 소유 액터의 UWxContextEffectsComponent 가 담당한다. 노티파이는 EffectTag 만 넘겨 트리거한다.
	if (const UWxContextEffectsComponent* ContextEffects = Owner->FindComponentByClass<UWxContextEffectsComponent>())
	{
		ContextEffects->TriggerEffect(EffectTag, Owner->GetActorLocation());
	}
}
