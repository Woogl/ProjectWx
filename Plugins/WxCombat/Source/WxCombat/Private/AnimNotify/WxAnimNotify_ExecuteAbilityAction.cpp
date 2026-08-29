// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_ExecuteAbilityAction.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "WxGameplayTags.h"

void UWxAnimNotify_ExecuteAbilityAction::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !ActionEventTag.MatchesTag(WxGameplayTags::Event_AbilityAction))
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.Instigator = Owner;
	Payload.EventTag = ActionEventTag;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, ActionEventTag, Payload);
}

FString UWxAnimNotify_ExecuteAbilityAction::GetNotifyName_Implementation() const
{
	if (ActionEventTag.IsValid())
	{
		return ActionEventTag.ToString();
	}

	return Super::GetNotifyName_Implementation();
}
