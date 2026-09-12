// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_UseItem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "WxGameplayTags.h"

void UWxAnimNotify_UseItem::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = WxGameplayTags::Event_UseItem;
	Payload.Instigator = Owner;
	Payload.OptionalObject = this;
	Payload.OptionalObject2 = MeshComp;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Payload.EventTag, Payload);
}
