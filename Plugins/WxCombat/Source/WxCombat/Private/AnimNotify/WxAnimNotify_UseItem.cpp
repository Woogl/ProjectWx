// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_UseItem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "WxGameplayTags.h"

void UWxAnimNotify_UseItem::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, WxGameplayTags::Event_UseItem, FGameplayEventData());
}
